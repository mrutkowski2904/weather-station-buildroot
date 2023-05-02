#include <linux/module.h>
#include <linux/kernel.h>

static int __init lps25hb_init(void)
{
    pr_info("lps25hb init\n");
    return 0;
}

static void __exit lps25hb_exit(void)
{

}

module_init(lps25hb_init);
module_exit(lps25hb_exit);

MODULE_LICENSE("GPL");