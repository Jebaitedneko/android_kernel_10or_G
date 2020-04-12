/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/hqsysfs.h>

#include "hqsys_misc.h"

#include "hq_emmc_name.h"
#include <linux/gpio.h>

#define HQ_SYS_FS_VER "2017-04-07 V0.3"

static HW_INFO(HWID_VER,ver);
static HW_INFO(HWID_SUMMARY,hw_summary);
//static HW_INFO(HWID_DDR,ram);
static HW_INFO(HWID_EMMC,emmc);
static HW_INFO(HWID_LCM,lcm);
//static HW_INFO(HWID_BIAS_IC,lcm_bias_ic);
static HW_INFO(HWID_CTP,ctp);
static HW_INFO(HWID_MAIN_CAM,main_cam);
//static HW_INFO(HWID_MAIN_LENS,main_cam_len);
//static HW_INFO(HWID_FLASHLIGHT,flashlight);
static HW_INFO(HWID_SUB_CAM,sub_cam);
static HW_INFO(HWID_GSENSOR,gsensor);
static HW_INFO(HWID_ALSPS,alsps);
static HW_INFO(HWID_MSENSOR,msensor);
static HW_INFO(HWID_GYRO,gyro);
//static HW_INFO(HWID_IRDA,irda);
//static HW_INFO(HWID_FUEL_GAUGE_IC,fuel_gauge_ic);
//static HW_INFO(HWID_NFC,nfc);
//static HW_INFO(HWID_FP,fingerprint);
//static HW_INFO(HWID_TEE,tee);
static HW_INFO(HWID_WIFI,wifi);
static HW_INFO(HWID_BT,bt);
static HW_INFO(HWID_GPS,gps);
//static HW_INFO(HWID_FM,fm);
static HW_INFO(HWID_FINGERPRINT,fingerprint);
static HW_INFO(HWID_VERSION,version);


static struct attribute *huaqin_attrs[] = {
	&hw_info_ver.attr,
	&hw_info_hw_summary.attr,
//	&hw_info_ram.attr,
	&hw_info_emmc.attr,
	&hw_info_lcm.attr,
//	&hw_info_lcm_bias_ic.attr,
	&hw_info_ctp.attr,
	&hw_info_main_cam.attr,
//	&hw_info_main_cam_len.attr,
//	&hw_info_flashlight.attr,
	&hw_info_sub_cam.attr,
	&hw_info_gsensor.attr,
	&hw_info_alsps.attr,
	&hw_info_msensor.attr,
	&hw_info_gyro.attr,
//	&hw_info_irda.attr,
//	&hw_info_fuel_gauge_ic.attr,
//	&hw_info_nfc.attr,
//	&hw_info_fingerprint.attr,
//	&hw_info_tee.attr,
    &hw_info_wifi.attr,
    &hw_info_bt.attr,
    &hw_info_gps.attr,
//    &hw_info_fm.attr,
    &hw_info_fingerprint.attr,
    &hw_info_version.attr,
	NULL
};

struct mmc_host *g_emmc_host;
extern EMMC_NAME_STRUCT emmc_names_table[];
extern int num_of_emmc_records;
static char device_name_emmc[100] = {"not found"};
static char device_name_alsps[100] = {"not found"};
static char device_name_gsensor[100] = {"not found"};
static char device_name_msensor[100] = {"not found"};
static char device_name_gyro[100] = {"not found"};
static char device_name_4in1[100] = {"WCN3615"};
static char device_stage[100] = {"Unknown Stage"}; 

static int get_stage_version(void)
{
	int i = 0;
	int len;
	int stage_gpio[4] = {-1,-1,-1, -1};
	int stage_val = 0;
	int ret = 0;
	static int init_done = false;
	struct device_node *node = NULL;
	node = of_find_compatible_node(NULL, NULL, "pcb-gpio");
	if (node) {
		len = of_property_read_u32_array(node, "stage-gpio",stage_gpio, ARRAY_SIZE(stage_gpio));
	}
	for (i = 0; i < 4; i++)
	{
		if (gpio_is_valid(stage_gpio[i])) {
			if (!init_done){
				ret = gpio_request(stage_gpio[i], "stage_gpio");
				if (ret) {
					pr_err("can't request gpio #%d: %d\n", i, ret);
					continue;
				}
			}
			ret = gpio_direction_input(stage_gpio[i]);
			if (ret) {
				printk("%s: failed to set %d direction as in", __func__, stage_gpio[i]);
				return ret;
			}
			stage_val |= __gpio_get_value(stage_gpio[i]) << i;
			printk("%s: pcb-gpios stage gpio:%d, val:%d, stage: %x\n", __func__, stage_gpio[i],__gpio_get_value(stage_gpio[i]), stage_val);
		}
	}
	init_done = true;

    switch(stage_val){
	    case HVT:
		    sprintf(device_stage, "HVT");
		    break;
		case EVT:
			sprintf(device_stage, "EVT1");
			break;
		case DVT1:
			sprintf(device_stage, "EVT2");
			break;
		case DVT2:
			sprintf(device_stage, "DVT");
			break;
		case PVT:
			sprintf(device_stage, "PVT");
			break;
		case MVT:
			sprintf(device_stage, "MVT");
			break;
		default:
			break;
	}

	return stage_val;
}

static ssize_t huaqin_show(struct kobject *kobj, struct attribute *a, char *buf)
{
	ssize_t count = 0;
	struct hw_info *hw = container_of(a, struct hw_info , attr);

	if(NULL == hw){
		return sprintf(buf, "Data error\n");
	}

	if(HWID_VER == hw->hw_id){
		count = sprintf(buf, "%s\n", HQ_SYS_FS_VER);
	} else if (HWID_VERSION== hw->hw_id) {
        get_stage_version();
		count = sprintf(buf, "%s\n" ,device_stage);
	}else if(HWID_SUMMARY == hw->hw_id){
		//iterate all device and output the detail
		int iterator = 0;
		struct hw_info *curent_hw = NULL;
		struct attribute *attr = huaqin_attrs[iterator];
	
		while(attr){
			curent_hw = container_of(attr, struct hw_info , attr);
			iterator += 1;
			attr = huaqin_attrs[iterator];

			if(curent_hw->hw_exist && (NULL != curent_hw->hw_device_name)){
				count += sprintf(buf+count, "%s: %s\n" ,curent_hw->attr.name,curent_hw->hw_device_name);
			} else if ((curent_hw->hw_id == HWID_WIFI) || (curent_hw->hw_id == HWID_BT) || (curent_hw->hw_id == HWID_GPS)) {
                count += sprintf(buf+count, "%s: %s\n" ,curent_hw->attr.name,device_name_4in1);
            } else if (curent_hw->hw_id == HWID_VERSION) {
                get_stage_version();
                count += sprintf(buf+count, "%s: %s\n" ,curent_hw->attr.name,device_stage);
            }
		}
	} else if ((hw->hw_id == HWID_WIFI) || (hw->hw_id == HWID_BT) || (hw->hw_id == HWID_GPS)){
        count = sprintf(buf, "%s\n" ,device_name_4in1);
    } else{
		if(0 == hw->hw_exist){
			count = sprintf(buf, "Not support\n");
		}else if(NULL == hw->hw_device_name){
			count = sprintf(buf, "Installed with no device Name\n");
		}else{
			count = sprintf(buf, "%s\n" ,hw->hw_device_name);
		}
	}

	return count;
}

static ssize_t show_flash(char *buf)
{
    int ret_value = 1;
	char temp_buf[10];

	int i = 0;
	if(!g_emmc_host) {
		ret_value = sprintf(buf, "EMMC: CANT DETECT mmc_host\n");
		goto ERROR_R;
	}
	if(!g_emmc_host->card) {
		ret_value = sprintf(buf, "EMMC: CANT DETECT emmc\n");
		goto ERROR_R;
	}

	printk("num_of_emmc_records :%d\n",num_of_emmc_records);

	temp_buf[0] = (g_emmc_host->card->raw_cid[0] >> 24) & 0xFF; /* Manufacturer ID */
	temp_buf[1] = (g_emmc_host->card->raw_cid[0] >> 16) & 0xFF; /* Reserved(6)+Card/BGA(2) */
	temp_buf[2] = (g_emmc_host->card->raw_cid[0] >> 8 ) & 0xFF; /* OEM/Application ID */
	temp_buf[3] = (g_emmc_host->card->raw_cid[0] >> 0 ) & 0xFF; /* Product name [0] */
	temp_buf[4] = (g_emmc_host->card->raw_cid[1] >> 24) & 0xFF; /* Product name [1] */
	temp_buf[5] = (g_emmc_host->card->raw_cid[1] >> 16) & 0xFF; /* Product name [2] */
	temp_buf[6] = (g_emmc_host->card->raw_cid[1] >> 8 ) & 0xFF; /* Product name [3] */
	temp_buf[7] = (g_emmc_host->card->raw_cid[1] >> 0 ) & 0xFF; /* Product name [4] */
	temp_buf[8] = (g_emmc_host->card->raw_cid[2] >> 24) & 0xFF; /* Product name [5] */
	temp_buf[9] = (g_emmc_host->card->raw_cid[2] >> 16) & 0xFF; /* Product revision */

	for(i = 0;i < num_of_emmc_records;i++){
		if (memcmp(temp_buf, emmc_names_table[i].ID, 9) == 0){
			ret_value = sprintf(buf, "%s",emmc_names_table[i].emmc_name);
			break;
		}
	}

	if(i == num_of_emmc_records){
		ret_value = sprintf(buf, "flash_name:Not Found\n");
	}

	printk("\n\ng_emmc_host->card->raw_cid[0]:0x%x\n",g_emmc_host->card->raw_cid[0]);
	printk("g_emmc_host->card->raw_cid[1]:0x%x\n",g_emmc_host->card->raw_cid[1]);
	printk("g_emmc_host->card->raw_cid[2]:0x%x\n",g_emmc_host->card->raw_cid[2]);
	printk("g_emmc_host->card->raw_cid[3]:0x%x\n\n",g_emmc_host->card->raw_cid[3]);

    return ret_value;

ERROR_R:
    return ret_value;
}

static ssize_t huaqin_store(struct kobject *kobj, struct attribute *a, const char *buf, size_t count)
{
    struct hw_info *hw = container_of(a, struct hw_info , attr);

    if(NULL == hw){
		return count;
	}

    switch(hw->hw_id){
        case HWID_GSENSOR:
            hw->hw_exist = 1;
            snprintf(device_name_gsensor, 100, "%s", buf);
            hw->hw_device_name = device_name_gsensor;
        break;

        case HWID_ALSPS:
            hw->hw_exist = 1;
            snprintf(device_name_alsps, 100, "%s", buf);
            hw->hw_device_name = device_name_alsps;
        break;

        case HWID_GYRO:
            hw->hw_exist = 1;
            snprintf(device_name_gyro, 100, "%s", buf);
            hw->hw_device_name = device_name_gyro;
        break;

        case HWID_MSENSOR:
            hw->hw_exist = 1;
            snprintf(device_name_msensor, 100, "%s", buf);
            hw->hw_device_name = device_name_msensor;
        break;

	    default:
		break;
    }

    return count;
}

/* huaqin object */
static struct kobject huaqin_kobj;
static const struct sysfs_ops huaqin_sysfs_ops = {
	.show = huaqin_show,
	.store = huaqin_store,
};

/* huaqin type */
static struct kobj_type huaqin_ktype = {
	.sysfs_ops = &huaqin_sysfs_ops,
	.default_attrs = huaqin_attrs
};

/* huaqin device class */
static struct class  *huaqin_class;
static struct device *huaqin_hw_device;


int register_kboj_under_hqsysfs(struct kobject *kobj, struct kobj_type *ktype, const char *fmt, ...){
	return kobject_init_and_add(kobj, ktype, &(huaqin_hw_device->kobj), fmt);
}

static int __init create_sysfs(void)
{
	int ret;

	/* create class (device model) */
	huaqin_class = class_create(THIS_MODULE, HUAQIN_CLASS_NAME);
	if (IS_ERR(huaqin_class)) {
		pr_err("%s fail to create class\n",__func__);
		return -1;
	}

	huaqin_hw_device = device_create(huaqin_class, NULL, MKDEV(0, 0), NULL, HUAIN_INTERFACE_NAME);
	if (IS_ERR(huaqin_hw_device)) {
		pr_warn("fail to create device\n");
		return -1;
	}
	
	/* add kobject */
	ret = kobject_init_and_add(&huaqin_kobj, &huaqin_ktype, &(huaqin_hw_device->kobj), HUAQIN_HWID_NAME);
	if (ret < 0) {
		pr_err("%s fail to add kobject\n",__func__);
		return ret;
	}

	return 0;
}

int hq_deregister_hw_info(enum hardware_id id,char *device_name){
	int ret = 0;
	int find_hw_id = 0;
	int iterator = 0;

	struct hw_info *hw = NULL;
	struct attribute *attr = huaqin_attrs[iterator];

	if(NULL == device_name){
		pr_err("[%s]: device_name does not allow empty\n",__func__);
		ret = -2;
		goto err;
	}
	
	while(attr){
		hw = container_of(attr, struct hw_info , attr);
		
		iterator += 1;
		attr = huaqin_attrs[iterator];
		
		if(NULL == hw){
			continue;
		}
		
		if(id == hw->hw_id){
			find_hw_id = 1;

			if(0 == hw->hw_exist){
				pr_err("[%s]: device has not registed hw->id:0x%x . Cant be deregistered\n"
					,__func__
					,hw->hw_id);
					
				ret = -4;
				goto err;
			}else if(NULL == hw->hw_device_name){

				pr_err("[%s]:hw_id is 0x%x Device name cant be NULL\n"
					,__func__
					,hw->hw_id);
				ret = -5;
				goto err;
			}
			else{
				if(0 == strncmp(hw->hw_device_name,device_name,strlen(hw->hw_device_name))){
					hw->hw_device_name = NULL;
					hw->hw_exist = 0;
				}else{
					pr_err("[%s]: hw_id is 0x%x Registered device name %s , want to deregister: %s\n"
						,__func__
						,hw->hw_id
						,hw->hw_device_name
						,device_name);
					ret = -6;
					goto err;
				}
			}

			goto err;

		}else
			continue;

	}

	if(0 == find_hw_id){
		pr_err("[%s]: Cant find correct hardware_id: 0x%x\n",__func__,id);
		ret = -3;
	}

err:
	return ret;

}


int hq_regiser_hw_info(enum hardware_id id,char *device_name){

	int ret = 0;
	int find_hw_id = 0;
	int iterator = 0;

	struct hw_info *hw = NULL;
	struct attribute *attr = huaqin_attrs[iterator];

	if(NULL == device_name){
		pr_err("[%s]: device_name does not allow empty\n",__func__);
		ret = -2;
		goto err;
	}

	while(attr){
		hw = container_of(attr, struct hw_info , attr);

		iterator += 1;
		attr = huaqin_attrs[iterator];
		
		if(NULL == hw){
			continue;
		}
		
		if(id == hw->hw_id){
			find_hw_id = 1;

			if(hw->hw_exist){
				pr_err("[%s]: device has already registed hw->id:0x%x hw_device_name:%s\n"
					,__func__
					,hw->hw_id
					,hw->hw_device_name);
				ret = -4;
				goto err;
			}

			switch(hw->hw_id){
				/*
			    case HWID_MAIN_CAM:
			    case HWID_SUB_CAM:
			    case HWID_MAIN_CAM_2:
			    case HWID_SUB_CAM_2:
                    if(map_cam_drv_to_vendor(device_name))
                        hw->hw_device_name = map_cam_drv_to_vendor(device_name);
                    else
                        hw->hw_device_name = "Can't find Camera Vendor";
			        break;
				*/
				case HWID_EMMC:
                    show_flash(device_name_emmc);
                    hw->hw_device_name = device_name_emmc;
                    break;
			    default:
			        hw->hw_device_name = device_name;
			        break;
			}

			
			hw->hw_exist = 1;
			goto err;

		}else
			continue;

	}

	if(0 == find_hw_id){
		pr_err("[%s]: Cant find correct hardware_id: 0x%x\n",__func__,id);
		ret = -3;
	}

err:
	return ret;
}

//#include <linux/hqsysfs.h>
//#include "hqsys_misc.h"
#include <linux/proc_fs.h>
#include <linux/module.h>
#include <linux/seq_file.h>

#define PROC_BOOT_REASON_FILE "boot_status"
static struct proc_dir_entry *boot_reason_proc = NULL;
static unsigned int boot_into_factory = 0;
static int boot_reason_proc_show(struct seq_file *file, void* data)
{
	char temp[40] = {0};

	sprintf(temp, "%d\n", boot_into_factory );
	seq_printf(file, "%s\n", temp);	
	return 0;
}

static int boot_reason_proc_open (struct inode* inode, struct file* file) 
{
	return single_open(file, boot_reason_proc_show, inode->i_private);
}

static const struct file_operations boot_reason_proc_fops =
{
	.open = boot_reason_proc_open,
	.read = seq_read,
};

static int __init get_boot_rease(char *str)
{
	if (strcmp("boot_with_factory", str) == 0) {
		boot_into_factory = 1;
	}
	return 0;
}

__setup("androidboot.boot_reason=", get_boot_rease);

static int __init hq_harware_init(void)
{
    /* create sysfs entry at /sys/class/huaqin/interface/hw_info */
	create_sysfs();

	boot_reason_proc = proc_create(PROC_BOOT_REASON_FILE, 0644, NULL, &boot_reason_proc_fops);
	if (boot_reason_proc == NULL)
	{
		pr_err("[%s]: create_proc_entry boot_reason_proc failed\n", __func__);
	}

    return 0;
}

core_initcall(hq_harware_init);
MODULE_AUTHOR("KaKa Ni <nigang@huaqin.com>");
MODULE_DESCRIPTION("Huaqin Hardware Info Driver");
MODULE_LICENSE("GPL");
