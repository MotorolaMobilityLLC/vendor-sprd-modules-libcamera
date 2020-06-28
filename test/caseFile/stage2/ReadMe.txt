1, hal.json文件为hal分层测试的输入参数文件；
2，drv.json文件为drv分层测试的输出参数文件；

其中:
"CaseID" : 0,
/* 为所测试的case id */

"ChipID" : 0,
/* 为所测试的chip id，需要与测试代码头文件定义匹配，参见sprd_cam_test.h */

"PathID" : 1,
/* 为所测试ip的path id，参见sprd_cam_test.h，若需要多条path同时测试需要考虑该参数扩展为数组形式
* 对于dcam而言：包括bin/full/AE/AF/pdaf等path信息；
* 对于isp而言：preview path需要配置为0，capture path为1，video为2
*/

"TestMode" : 1,
/* 为所测试ip的模式，对于dcam来说：0表示online，1表示offline；
* 对于isp而言： 0表示cfg模式，1表示ap模式；
*/

"ParmPath" : "/data/vendor/cameraserver/testdcam/",
/* 为所测试ip的对应cmodle参数的txt文件集路径，对应文件在本目录的testxx下，分多个项目 */

"ImagePath" : "/data/vendor/cameraserver/testimage/xx.raw"
/* 为所测试ip对应的图像输入输出数据路径, 若需要测试多帧可以考虑多个case id组合，
* 若一次测试需要输出多幅图像，需要考虑将改参数扩展为数组形式
*/


testdcam文件夹中为dcam对应的drv需要测试的参数文件；
testisp文件夹中为isp对应的drv需要测试的参数文件；
