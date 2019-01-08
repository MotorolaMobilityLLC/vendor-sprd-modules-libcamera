
#include <linux/cdev.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/debugfs.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/sprd_ion.h>
#include <linux/mfd/syscon.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/sched.h>
#include <linux/semaphore.h>
#include <linux/slab.h>
#include <linux/sprd_iommu.h>
#include <linux/sprd_ion.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/wait.h>
#include <linux/notifier.h>
#include <linux/compat.h>
#include "sprd_vdsp.h"
#include <asm/cacheflush.h>
#include <linux/firmware.h>
#include <linux/crc32.h>
#include <linux/dma-mapping.h>
#include "sprd_vdsp_cmd.h"
#include <linux/dma-buf.h>

static const struct of_device_id of_match_table_vdsp_edp[] = {
	{.compatible = "sprd,roc1-vdsp-edp"},
	{},
};


	struct sprd_iommu_map_data iommu_data;
	struct sprd_iommu_unmap_data iommu_udata;
	struct device device_edp;
static int vdsp_edp_probe(struct platform_device *pdev)
{
	//int ret;
	struct device_node *node = pdev->dev.of_node;
	const struct of_device_id *of_id;
	//const char *str;
	//u32 vdsp_share_buff_addr;
	//size_t  buf_size = 0x100000;
	//int fd = 0;
	//struct dma_buf *dmabuf = NULL;
	//unsigned long phyaddr;
	//int ret;
	//u32 *buffer;
	//void *kvaddr;
	//struct sprd_iommu_map_data iommu_data;
	pr_emerg("vdsp_edp_probe called !\n");

	of_id = of_match_node(of_match_table_vdsp_edp, node);
#if 0
	fd = ion_alloc((size_t)(buf_size), ION_HEAP_ID_MASK_SYSTEM, 0);
	if (fd < 0) {
		pr_emerg("ion_alloc buffer fail(fd=%d)\n",fd);
		return fd;
	}
	pr_emerg("fd:%d\n",fd);
#if 0
	ret = sprd_ion_get_phys_addr(fd, dmabuf, &phyaddr, &buf_size);
	*buffer = (u32)phyaddr;
	pr_emerg("test3 allocated for vdsp buffer:0x%x, type %d\n",
	      (u32)phyaddr,ION_HEAP_ID_MASK_FB);
#endif

#if 1
	dmabuf = dma_buf_get(fd);
	kvaddr = sprd_ion_map_kernel(dmabuf,0);
	dma_buf_put(dmabuf);
#endif
	 ret = sprd_ion_get_buffer(fd, NULL,
                                &(iommu_data.buf),
                                &iommu_data.iova_size);

        iommu_data.ch_type = SPRD_IOMMU_FM_CH_RW;
#endif	
	device_edp = pdev->dev;
	return 0;

}


int vdsp_edp_iommu_map(void)
{
	struct dma_buf *dmabuf = NULL;
	int ret;
	size_t  buf_size = 0x30000;
	int fd = 0;
	void *kvaddr;
	unsigned long phyaddr;

	pr_info("vdsp_map  !\n");
	fd = ion_alloc((size_t)(buf_size), /*ION_HEAP_ID_MASK_SYSTEM*/ION_HEAP_ID_MASK_FB, 0);
	if (fd < 0) {
		pr_emerg("ion_alloc buffer fail(fd=%d)\n",fd);
		return fd;
	}
       ret = sprd_ion_get_phys_addr(fd, dmabuf, &phyaddr, &buf_size);
	pr_emerg("test3 allocated for vdsp buffer:0x%x, type %d\n",
	      (u32)phyaddr,ION_HEAP_ID_MASK_FB);
 

	pr_emerg("fd:%d\n",fd);
	dmabuf = dma_buf_get(fd);
	kvaddr = sprd_ion_map_kernel(dmabuf,0);
	dma_buf_put(dmabuf);
	pr_emerg("phy addr:%llx\n",virt_to_phys(kvaddr));
	 ret = sprd_ion_get_buffer(fd, NULL,
                                &(iommu_data.buf),
                                &iommu_data.iova_size);

        iommu_data.ch_type = SPRD_IOMMU_FM_CH_RW;
        sprd_iommu_map(&device_edp, &iommu_data);
	pr_emerg("vdsp_edp_map (iommu_data.iova_addr =%lx)\n",iommu_data.iova_addr);

		iommu_udata.iova_size = iommu_data.iova_size;
                iommu_udata.iova_addr = iommu_data.iova_addr;
                iommu_udata.ch_type = SPRD_IOMMU_FM_CH_RW;
	
	//sprd_iommu_unmap(&device_edp, &iommu_udata);
	pr_info("vdsp_map Success !\n");
	return 0;
}

static int vdsp_edp_remove(struct platform_device *pdev)
{
	pr_info("vdsp_remove Success !\n");
	return 0;
}

static struct platform_driver vdsp_edp_driver = {
	.probe = vdsp_edp_probe,
	.remove = vdsp_edp_remove,

	.driver = {
		   .owner = THIS_MODULE,
		   .name = "sprd_vdsp_edp",
		   .of_match_table = of_match_ptr(of_match_table_vdsp_edp),
		   },
};


static int __init sprd_vdsp_edp_init(void)
{
	int result = 0;

	result = platform_driver_register(&vdsp_edp_driver);
	pr_info("%s,result:%d\n", __func__, result);
	return result;
}

static void __exit sprd_vdsp_edp_exit(void)
{
	platform_driver_unregister(&vdsp_edp_driver);
}

late_initcall(sprd_vdsp_edp_init);
MODULE_DESCRIPTION("SPRD VDSP EDP Driver");
MODULE_LICENSE("GPL");
