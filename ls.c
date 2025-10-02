/*#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"

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
*/

#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include <stdio.h>

char*
fmtname(char *path)
{
  static char buf[DIRSIZ+1];
  char *p;

  for(p=path+strlen(path); p >= path && *p != '/'; p--)
    ;
  p++;

  if(strlen(p) >= DIRSIZ)
    return p;
  memmove(buf, p, strlen(p));
  memset(buf+strlen(p), ' ', DIRSIZ-strlen(p));
  return buf;
}

// Simple wildcard match: '*' matches any string
int
match(char *pattern, char *name)
{
  char *p = pattern, *n = name;

  while(*p && *n){
    if(*p == '*'){
      p++;
      if(*p == 0) return 1; // trailing *, matches everything
      while(*n){
        if(match(p, n)) return 1;
        n++;
      }
      return 0;
    } else {
      if(*p != *n) return 0;
      p++; n++;
    }
  }
  return (*p == 0 && *n == 0);
}


 void
lsdir(char *dir, char *pattern)
{
  char buf[512], name[DIRSIZ+1];
  int fd;
  struct dirent de;
  struct stat st;

  if((fd = open(dir, 0)) < 0){
    printf(2, "ls: cannot open %s\n", dir);
    return;
  }

  while(read(fd, &de, sizeof(de)) == sizeof(de)){
    if(de.inum == 0) continue;

    // null-terminate de.name
    int i;
    for(i=0; i<DIRSIZ && de.name[i]; i++)
      name[i] = de.name[i];
    name[i] = 0;

    if(!match(pattern, name)) continue;

    // safely build full path
    if(strcmp(dir, ".") == 0)
      sprintf(buf, "%s", name);
    else
      sprintf(buf, "%s/%s", dir, name);

    if(stat(buf, &st) < 0){
      printf(1, "ls: cannot stat %s\n", buf);
      continue;
    }

    printf(1, "%s %d %d %d\n", fmtname(buf), st.type, st.ino, st.size);
  }
  close(fd);
}



void
ls(char *path)
{
    char *star = strchr(path, '*');

  if(star){
    // Determine directory and pattern
    char dir[512], pattern[DIRSIZ+1];
    char *slash = path + strlen(path) - 1;
    while(slash >= path && *slash != '/') slash--;

    if(slash < path){  // no slash, current dir
      strcpy(dir, ".");
      strcpy(pattern, path);
    } else {  // has slash
      int dirlen = slash - path;
      memmove(dir, path, dirlen);
      dir[dirlen] = 0;
      strcpy(pattern, slash + 1);
    }

    lsdir(dir, pattern);
    return;
  }

  // Normal file/dir
  int fd;
  struct stat st;
  char buf[512], *p;
  struct dirent de;

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

