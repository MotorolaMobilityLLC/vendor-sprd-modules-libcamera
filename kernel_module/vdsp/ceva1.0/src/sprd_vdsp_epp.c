
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

static const struct of_device_id of_match_table_vdsp_epp[] = {
	{.compatible = "sprd,roc1-vdsp-epp"},
	{},
};


static int vdsp_epp_probe(struct platform_device *pdev)
{
	//int ret;
	struct device_node *node = pdev->dev.of_node;
	const struct of_device_id *of_id;
	//const char *str;
	//u32 vdsp_share_buff_addr;
	size_t  buf_size = 0x100000;
	int fd = 0;
	struct dma_buf *dmabuf = NULL;
	//unsigned long phyaddr;
	int ret;
	//u32 *buffer;
	void *kvaddr;
	struct sprd_iommu_map_data iommu_data;
	pr_emerg("vdsp_epp_probe called !\n");

	of_id = of_match_node(of_match_table_vdsp_epp, node);

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
	pr_emerg("vdsp_epp_probe call0 !\n");
	dmabuf = dma_buf_get(fd);
	pr_emerg("vdsp_epp_probe call1(dmabuf =%p)\n",dmabuf);
	kvaddr = sprd_ion_map_kernel(dmabuf,0);
	pr_emerg("vdsp_epp_probe call2(kvaddr=%p)\n",kvaddr);
	dma_buf_put(dmabuf);
	pr_emerg("vdsp_epp_probe call3(dmabuf =%p)\n",dmabuf);
#endif
	 ret = sprd_ion_get_buffer(fd, NULL,
                                &(iommu_data.buf),
                                &iommu_data.iova_size);

        iommu_data.ch_type = SPRD_IOMMU_FM_CH_RW;
#if 0	
        ret = sprd_iommu_map(&pdev->dev, &iommu_data);
	pr_emerg("vdsp_epp_probe call3(iommu_data.iova_addr =%lx)\n",iommu_data.iova_addr);
#endif	
	return 0;

}

static int vdsp_epp_remove(struct platform_device *pdev)
{
	pr_info("vdsp_remove Success !\n");
	return 0;
}

static struct platform_driver vdsp_epp_driver = {
	.probe = vdsp_epp_probe,
	.remove = vdsp_epp_remove,

	.driver = {
		   .owner = THIS_MODULE,
		   .name = "sprd_vdsp_epp",
		   .of_match_table = of_match_ptr(of_match_table_vdsp_epp),
		   },
};


static int __init sprd_vdsp_epp_init(void)
{
	int result = 0;

	result = platform_driver_register(&vdsp_epp_driver);
	pr_info("%s,result:%d\n", __func__, result);
	return result;
}

static void __exit sprd_vdsp_epp_exit(void)
{
	platform_driver_unregister(&vdsp_epp_driver);
}

late_initcall(sprd_vdsp_epp_init);
MODULE_DESCRIPTION("SPRD VDSP EPP Driver");
MODULE_LICENSE("GPL");
