// init: The initial user-level program

#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

char *argv[] = { "sh", 0 };

// Simple user database
struct user {
  char *username;
  char *password;
} users[] = {
  {"nermine", "2006"},
  {"nour", "2005"},
};

int
login(void)
{
  char uname[32], pword[32];
  int i;

  while(1){
    printf(1, "Username: ");
    gets(uname, sizeof(uname));
    uname[strlen(uname)-1] = 0;  // remove newline

    printf(1, "Password: ");
    gets(pword, sizeof(pword));
    pword[strlen(pword)-1] = 0;

    for(i = 0; i < sizeof(users)/sizeof(users[0]); i++){
      if(strcmp(uname, users[i].username) == 0 &&
         strcmp(pword, users[i].password) == 0){
        printf(1, "Login successful!\n");
        return 1;
      }
    }
    printf(1, "Invalid login, try again.\n");
  }
}

int
main(void)
{
  int pid, wpid;

  if(open("console", O_RDWR) < 0){
    mknod("console", 1, 1);
    open("console", O_RDWR);
  }
  dup(0);  // stdout
  dup(0);  // stderr

  for(;;){
    // Run login before starting shell
    if(login()){
      printf(1, "init: starting sh\n");
    }

    pid = fork();
    if(pid < 0){
      printf(1, "init: fork failed\n");
      exit();
    }
    if(pid == 0){
      exec("sh", argv);
      printf(1, "init: exec sh failed\n");
      exit();
    }
    while((wpid=wait()) >= 0 && wpid != pid)
      printf(1, "zombie!\n");
  }
}

