#define LOG_TAG "libloader"

#include <fstream>
#include <map>
#include <thread>
#include <vector>
#include <dlfcn.h>
#include <log/log.h>

#include <libloader.h>

using namespace std;

static const char *kPreloadLibraryOverride =
    "/data/vendor/cameraserver/preload.txt";
static const char *kPreloadLibraries[] = {
    "libispalg.so",
    "libjpeg_hw_sprd.so",
    "libawb.so",
    "libawb1.so"};

class LibraryLoader {
  public:
    LibraryLoader() : mNext(0) {
        prepare();
        mThread = thread(&LibraryLoader::open, this);
    }

    ~LibraryLoader() {
        if (mThread.joinable())
            mThread.join();

        auto libraries = collect();
        for (auto lib : libraries)
            doClose(lib);
    }

    void *getLibraryHandle(string name) {
        void *handle = nullptr;
        library_status status;

        do {
            status = getLibraryStatus(name, &handle);

            if (status == Error)
                return nullptr;

            if (status == Available)
                return handle;

            if (status == Obtained)
                return doOpen(name);

            wait();
        } while (!finished());

        return doOpen(name);
    }

    void putLibraryHandle(void *handle) {
        if (!tryPutLibraryHandle(handle))
            doClose(handle);
    }

  private:
    typedef enum {
        Scheduled,
        Preparing,
        Available,
        Obtained,
        Error,
    } library_status;

    struct library_info {
        string name;
        void *handle;
        library_status status;

        library_info(string name)
            : name(name), handle(nullptr), status(Scheduled) {}
    };

    void prepare() {
        ifstream f(kPreloadLibraryOverride);
        if (f.good()) {
            /* use preload.txt for customization or debug purpose */
            while (!f.eof()) {
                string s;
                f >> s;
                if (s.length())
                    appendLibrary(s);
            }
            f.close();
        } else {
            for (auto n : kPreloadLibraries)
                appendLibrary(n);
        }

        ALOGI("got %zu library(s) on preload list", getLibraryCount());
    }

    void appendLibrary(string name) {
        lock_guard<mutex> l(mLibraryMutex);
        /* filter out duplicated */
        if (findLibraryInfoLocked(name) == mLibraries.end())
            mLibraries.push_back(library_info(name));
    }

    size_t getLibraryCount() {
        lock_guard<mutex> l(mLibraryMutex);
        return mLibraries.size();
    }

    void open() {
        string name;
        int pos;

        while ((pos = next(name)) >= 0) {
            void *handle = doOpen(name);
            updateLibrary(pos, handle, handle ? Available : Error);

            notify();
        }

        notify();
        ALOGI("loading finished");
    }

    int next(string &name) {
        lock_guard<mutex> l(mLibraryMutex);
        if (mNext < mLibraries.size()) {
            name = mLibraries[mNext].name;
            mLibraries[mNext].status = Preparing;
            return mNext++;
        }

        name = "";
        return -1;
    }

    void updateLibrary(size_t pos, void *handle, library_status status) {
        lock_guard<mutex> l(mLibraryMutex);
        mLibraries[pos].handle = handle;
        mLibraries[pos].status = status;
    }

    vector<void *> collect() {
        vector<void *> result;
        lock_guard<mutex> l(mLibraryMutex);
        for (auto &info : mLibraries) {
            if (info.status == Available)
                result.push_back(info.handle);
            /* mark status as Error to let user dlclose himself */
            info.status = Error;
        }

        return result;
    }

    library_status getLibraryStatus(string name, void **pHandle) {
        lock_guard<mutex> l(mLibraryMutex);
        auto it = findLibraryInfoLocked(name);
        if (it != mLibraries.end()) {
            if (it->status == Available) {
                it->status = Obtained;
                *pHandle = it->handle;
                return Available;
            }

            return it->status;
        }

        if (mNext < mLibraries.size()) {
            mLibraries.insert(mLibraries.begin() + mNext, library_info(name));
            return Scheduled;
        }

        /* there's no meaning to wait for loading thread while it's performing
         * dlopen on last library, we do it ourself and wait in dlopen call */
        return Obtained;
    }

    vector<library_info>::iterator findLibraryInfoLocked(string name) {
        return find_if(
            mLibraries.begin(), mLibraries.end(),
            [&](const library_info &info) { return name == info.name; });
    }

    bool finished() {
        lock_guard<mutex> l(mLibraryMutex);
        return mLibraries.size() >= mNext;
    }

    bool tryPutLibraryHandle(void *handle) {
        lock_guard<mutex> l(mLibraryMutex);
        auto it = find_if(
            mLibraries.begin(), mLibraries.end(),
            [=](const library_info &info) { return info.handle == handle; });

        if (it != mLibraries.end() && it->status == Obtained) {
            it->status = Available;
            return true;
        }

        return false;
    }

    void *doOpen(string name) {
        void *handle = dlopen(name.c_str(), RTLD_LAZY);
        if (!handle)
            ALOGE("fail to open lib '%s', error %s", name.c_str(), dlerror());
        else
            ALOGI("succeed to open lib '%s' %p", name.c_str(), handle);

        return handle;
    }

    void doClose(void *handle) {
        dlclose(handle);

        ALOGI("succeed to close lib %p", handle);
    }

    void wait() {
        unique_lock<mutex> l(mMutex);
        mCond.wait(l);
    }

    void notify() { mCond.notify_all(); }

    vector<library_info> mLibraries;
    size_t mNext;
    mutex mLibraryMutex;
    thread mThread;
    mutex mMutex;
    condition_variable mCond;
};

static LibraryLoader sLoader;

void *get_lib_handle(const char *lib_name) {
    return sLoader.getLibraryHandle(string(lib_name));
}
void put_lib_handle(void *handle) { return sLoader.putLibraryHandle(handle); }
