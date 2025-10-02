#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"

static int
has_wildcard(const char *s)
{
  for(; *s; s++)
    if(*s == '*')
      return 1;
  return 0;
}

static int
match(const char *name, const char *pattern)
{
  while(*pattern){
    if(*pattern == '*'){
      pattern++;
      if(*pattern == 0)
        return 1;
      while(*name){
        if(match(name, pattern))
          return 1;
        name++;
      }
      return 0;
    }
    if(*name != *pattern)
      return 0;
    name++;
    pattern++;
  }
  return *name == 0;
}

char*
fmtname(char *path)
{
  static char buf[DIRSIZ+1];
  char *p;

  // Find first character after last slash.
  for(p=path+strlen(path); p >= path && *p != '/'; p--)
    ;
  p++;

  // Return blank-padded name.
  if(strlen(p) >= DIRSIZ)
    return p;
  memmove(buf, p, strlen(p));
  memset(buf+strlen(p), ' ', DIRSIZ-strlen(p));
  return buf;
}

void
ls(char *path)
{
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;

  // Handle simple wildcard patterns like "*.c" or "dir/*.c"
  if(has_wildcard(path)){
    char dir[512];
    char pattern[512];
    char *q;

    // Find first character after last slash (directory separator)
    for(q = path + strlen(path); q >= path && *q != '/'; q--)
      ;
    q++;  // q now points to the start of the last path component (pattern)

    if(q == path){
      // No slash in path; use current directory
      strcpy(dir, ".");
    } else {
      int dirlen = q - path - 1; // exclude the '/'
      if(dirlen == 0 && path[0] == '/'){
        // pattern like "/*.c" -> directory is "/"
        strcpy(dir, "/");
      } else {
        if(dirlen >= sizeof(dir)){
          printf(1, "ls: path too long\n");
          return;
        }
        memmove(dir, path, dirlen);
        dir[dirlen] = 0;
      }
    }

    if(strlen(q) >= sizeof(pattern)){
      printf(1, "ls: path too long\n");
      return;
    }
    strcpy(pattern, q);

    if((fd = open(dir, 0)) < 0){
      printf(2, "ls: cannot open %s\n", dir);
      return;
    }

    if(fstat(fd, &st) < 0){
      printf(2, "ls: cannot stat %s\n", dir);
      close(fd);
      return;
    }

    if(st.type != T_DIR){
      printf(1, "ls: not a directory %s\n", dir);
      close(fd);
      return;
    }

    if(strlen(dir) + 1 + DIRSIZ + 1 > sizeof buf){
      printf(1, "ls: path too long\n");
      close(fd);
      return;
    }

    strcpy(buf, dir);
    p = buf+strlen(buf);
    *p++ = '/';
    while(read(fd, &de, sizeof(de)) == sizeof(de)){
      if(de.inum == 0)
        continue;

      char namebuf[DIRSIZ+1];
      memmove(namebuf, de.name, DIRSIZ);
      namebuf[DIRSIZ] = 0;
      if(!match(namebuf, pattern))
        continue;

      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;
      if(stat(buf, &st) < 0){
        printf(1, "ls: cannot stat %s\n", buf);
        continue;
      }
      printf(1, "%s %d %d %d\n", fmtname(buf), st.type, st.ino, st.size);
    }
    close(fd);
    return;
  }

  if((fd = open(path, 0)) < 0){
    printf(2, "ls: cannot open %s\n", path);
    return;
  }

  if(fstat(fd, &st) < 0){
    printf(2, "ls: cannot stat %s\n", path);
    close(fd);
    return;
  }

  switch(st.type){
  case T_FILE:
    printf(1, "%s %d %d %d\n", fmtname(path), st.type, st.ino, st.size);
    break;

  case T_DIR:
    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
      printf(1, "ls: path too long\n");
      break;
    }
    strcpy(buf, path);
    p = buf+strlen(buf);
    *p++ = '/';
    while(read(fd, &de, sizeof(de)) == sizeof(de)){
      if(de.inum == 0)
        continue;
      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;
      if(stat(buf, &st) < 0){
        printf(1, "ls: cannot stat %s\n", buf);
        continue;
      }
      printf(1, "%s %d %d %d\n", fmtname(buf), st.type, st.ino, st.size);
    }
    break;
  }
  close(fd);
}

int
main(int argc, char *argv[])
{
  int i;

  if(argc < 2){
    ls(".");
    exit();
  }
  for(i=1; i<argc; i++)
    ls(argv[i]);
  exit();
}

