#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include <string.h>
#include "threads/synch.h"

/* Lock for file system calls. */
static struct lock file_lock;

static void syscall_handler (struct intr_frame *);
static void valid_up (const void *);
static void validate (const void *, size_t);
static void validate_string (const char *);

static int
halt (void *esp)
{
  power_off ();
}

int
exit (void *esp)
{
  int status = 0;
  if (esp != NULL){
    validate (esp, sizeof(int));
    status = *((int *)esp);
    esp += sizeof (int);
  }
  else {
    status = -1;
  }
  
  char *name = thread_current ()->name, *save;
  name = strtok_r (name, " ", &save);
  
  printf ("%s: exit(%d)\n", name, status);
  thread_exit ();
}

static int
exec (void *esp)
{
  thread_exit ();
}

static int
wait (void *esp)
{
  thread_exit ();
}

static int
create (void *esp)
{
  
}

static int
remove (void *esp)
{
  thread_exit ();
}

static int
open (void *esp)
{
  validate (esp, sizeof(char *));
  const char *file_name = *((char **) esp);
  esp += sizeof (char *);

  validate_string (file_name);
  
  lock_acquire (&file_lock);
  struct file *f = filesys_open (file_name);
  lock_release (&file_lock);

  if (f == NULL)
    return -1;
  
  struct thread *t = thread_current ();

  int i;
  for (i = 2; i<MAX_FILES; i++)
  {
    if (t->files[i] == NULL){
      t->files[i] = f;
      break;
    }
  }

  if (i == MAX_FILES)
    return -1;
  else
    return i;
}

static int
filesize (void *esp)
{
  validate (esp, sizeof(int));
  int fd = *((int *) esp);
  esp += sizeof (int);

  struct thread *t = thread_current ();

  if (t->files[fd] == NULL)
    return -1;
  
  lock_acquire (&file_lock);
  int size = file_length (t->files[fd]);
  lock_release (&file_lock);

  return size;
}

static int
read (void *esp)
{
  thread_exit ();
}

static int
write (void *esp)
{
  validate (esp, sizeof(int));
  int fd = *((int *)esp);
  esp += sizeof (int);

  validate (esp, sizeof(void *));
  const void *buffer = *((void **) esp);
  esp += sizeof (void *);

  validate (esp, sizeof(unsigned));
  unsigned size = *((unsigned *) esp);
  esp += sizeof (unsigned);
  
  validate (buffer, size);
  
  struct thread *t = thread_current ();
  if (fd == STDOUT_FILENO)
  {
    /* putbuf (buffer, size); */
    lock_acquire (&file_lock);

    int i;
    for (i = 0; i<size; i++)
      putchar (*((char *) buffer + i));

    lock_release (&file_lock);
    return i;
  }
  else if (fd >=2 && t->files[fd] != NULL)
  {
    lock_acquire (&file_lock);
    int written = file_write (t->files[fd], buffer, size);
    lock_release (&file_lock);
    return written;
  }
  return 0;
}

static int
seek (void *esp)
{
  thread_exit ();
}

static int
tell (void *esp)
{
  thread_exit ();
}

static int
close (void *esp)
{
  thread_exit ();
}

static int
mmap (void *esp)
{
  thread_exit ();
}

static int
munmap (void *esp)
{
  thread_exit ();
}

static int
chdir (void *esp)
{
  thread_exit ();
}

static int
mkdir (void *esp)
{
  thread_exit ();
}

static int
readdir (void *esp)
{
  thread_exit ();
}

static int
isdir (void *esp)
{
  thread_exit ();
}

static int
inumber (void *esp)
{
  thread_exit ();
}

static int (*syscalls []) (void *) =
  {
    halt,
    exit,
    exec,
    wait,
    create,
    remove,
    open,
    filesize,
    read,
    write,
    seek,
    tell,
    close,

    mmap,
    munmap,

    chdir,
    mkdir,
    readdir,
    isdir,
    inumber
  };

const int num_calls = sizeof (syscalls) / sizeof (syscalls[0]);

void
syscall_init (void) 
{
  lock_init (&file_lock);
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall"); 
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  void *esp = f->esp;

  validate (esp, sizeof(int));
  int syscall_num = *((int *) esp);
  esp += sizeof(int);

  /* printf("\nSys: %d", syscall_num); */

  /* Just for sanity, we will anyway be checking inside all functions. */ 
  validate (esp, sizeof(void *));

  if (syscall_num >= 0 && syscall_num < num_calls)
  {
    int (*function) (void *) = syscalls[syscall_num];
    int ret = function (esp);
    f->eax = ret;
  }
  else
  {
    /* TODO:: Raise Exception */
    printf ("\nError, invalid syscall number.");
    thread_exit ();
  }
}

static void
validate_string (const char *s)
{
  validate (s, sizeof(char));
  while (*s != '\0')
    validate (s++, sizeof(char));
}

static void
validate (const void *ptr, size_t size)
{
  valid_up (ptr);
  if(size != 1)
    valid_up (ptr + size - 1);
}

static void
valid_up (const void *ptr)
{
  uint32_t *pd = thread_current ()->pagedir;
  if ( ptr == NULL || !is_user_vaddr (ptr) || pagedir_get_page (pd, ptr) == NULL)
  {
    exit (NULL);
  }
}
