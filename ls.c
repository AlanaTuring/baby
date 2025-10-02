#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"

// Simple '*' wildcard matcher: '*' matches any sequence (including empty)
static int
match_pattern(const char *pattern, const char *text)
{
  // If we've reached the end of the pattern, it matches only if text also ended
  if(*pattern == 0)
    return *text == 0;

  if(*pattern == '*'){
    // Skip consecutive '*'
    while(*pattern == '*')
      pattern++;
    // If pattern ends with '*', it matches the rest of text
    if(*pattern == 0)
      return 1;
    // Try to match the remainder of pattern at every position in text
    for(; *text; text++){
      if(match_pattern(pattern, text))
        return 1;
    }
    // Also try empty match against end of text
    return match_pattern(pattern, text);
  }

  // Regular character must match exactly
  if(*text && *pattern == *text)
    return match_pattern(pattern+1, text+1);

  return 0;
}

static int
has_star_in_last_component(const char *path)
{
  const char *lastslash = 0;
  const char *s;
  for(s = path; *s; s++)
    if(*s == '/')
      lastslash = s;
  const char *last = lastslash ? lastslash + 1 : path;
  for(; *last; last++)
    if(*last == '*')
      return 1;
  return 0;
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

  // Handle wildcard in the last path component by expanding within its directory
  if(has_star_in_last_component(path)){
    char dir[512];
    const char *s;
    const char *lastslash = 0;
    for(s = path; *s; s++)
      if(*s == '/')
        lastslash = s;

    const char *pattern = lastslash ? lastslash + 1 : path;

    if(lastslash){
      int dlen = lastslash - path;
      if(dlen >= sizeof(dir)){
        printf(1, "ls: path too long\n");
        return;
      }
      memmove(dir, path, dlen);
      dir[dlen] = 0;
    } else {
      dir[0] = '.';
      dir[1] = 0;
    }

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
      // Parent is not a directory; nothing to expand
      close(fd);
      return;
    }

    if(strlen(dir) + 1 + DIRSIZ + 1 > sizeof buf){
      printf(1, "ls: path too long\n");
      close(fd);
      return;
    }
    strcpy(buf, dir);
    p = buf + strlen(buf);
    *p++ = '/';
    while(read(fd, &de, sizeof(de)) == sizeof(de)){
      if(de.inum == 0)
        continue;

      char name[DIRSIZ+1];
      memmove(name, de.name, DIRSIZ);
      name[DIRSIZ] = 0;
      if(!match_pattern(pattern, name))
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

