// Öğrenci No: <BURAYA YAZIN>
// Ad Soyad: <BURAYA YAZIN>

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>

#define MAXARGS 16
#define MAXLINE 4096

// Komut yapıları //

struct cmd { int type; };

struct execcmd {
    int type;
    char *argv[MAXARGS];
};

struct redircmd {
    int type;
    struct cmd *cmd;
    char *file;
    int mode;
    int fd;
};

struct pipecmd {
    int type;
    struct cmd *left;
    struct cmd *right;
};

struct backcmd {
    int type;
    struct cmd *cmd;
};

// Fonksiyon prototipleri
int fork1(void);
struct cmd *parsecmd(char*);

// ------------------------------------------------------------
//                      runcmd()
// ------------------------------------------------------------
// *** SORU 1, 2, 3, 5 ÇÖZÜMLERİ BU FONKSİYONDA YAPILACAK ***

void runcmd(struct cmd *cmd)
{
    int p[2];
    int r;

    struct execcmd  *ecmd;
    struct pipecmd  *pcmd;
    struct redircmd *rcmd;

    if(cmd == 0)
        exit(0);

    switch(cmd->type){

    default:
        fprintf(stderr, "unknown runcmd\n");
        exit(-1);

    case ' ':
        ecmd = (struct execcmd*)cmd;
    	if(ecmd->argv[0] == 0)
      		exit(0);
    // -------------------------------------------------------
    // SORU 1: Basit komutlar için yazılacak kodlar buraya ...
    // -------------------------------------------------------
        execvp(ecmd->argv[0], ecmd->argv);
        // execvp sadece hata durumunda geri döner
        fprintf(stderr, "Hata: '%s' komutu calistirilamadi\n", ecmd->argv[0]);
        exit(1); // Hata durumunda alt süreci sonlandır

        break;

    case '<':
    case '>':
    case '2':   // 2> stderr yönlendirme

        rcmd = (struct redircmd*)cmd;
        
    // ----------------------------------------------------------------------------------------
    // SORU 2 ve SORU 5: I/O komutlar ve Hata Yönlendirme (2>) için yazılacak kodlar buraya ...
    // ----------------------------------------------------------------------------------------
    
        // İlgili dosya tanımlayıcısını (0, 1, veya 2) kapat
        close(rcmd->fd); 
        
        // Dosyayı aç. Bu dosya, kapatılan fd'nin yerini alacak.
        if(open(rcmd->file, rcmd->mode, 0666) < 0){
            fprintf(stderr, "Hata: '%s' dosyasi acilamadi\n", rcmd->file);
            exit(1);
        }
        
        // Yönlendirme tamam, şimdi içteki komutu çalıştır
    	runcmd(rcmd->cmd);

        break;
        
    case '|':
        pcmd = (struct pipecmd*)cmd;
        
    // ------------------------------------------------------------
    // SORU 3: Sıralı komutlar (|) için yazılacak kodlar buraya ...
    // -------------------------------------------------------------

        if(pipe(p) < 0){
            perror("pipe");
            exit(1);
        }

        // Sol komut (Çocuk 1)
        if(fork1() == 0){
            close(1);       // STDOUT'u kapat
            dup(p[1]);      // STDOUT'u borunun yazma ucuna yönlendir
            close(p[0]);    // Borunun uçlarını bu çocukta kapat
            close(p[1]);
            runcmd(pcmd->left);
        }

        // Sağ komut (Çocuk 2)
        if(fork1() == 0){
            close(0);       // STDIN'i kapat
            dup(p[0]);      // STDIN'i borunun okuma ucuna yönlendir
            close(p[0]);    // Borunun uçlarını bu çocukta kapat
            close(p[1]);
            runcmd(pcmd->right);
        }

        // Ebeveyn (Ana süreç)
        close(p[0]); // Borunun iki ucunu da ebeveynde kapat
        close(p[1]);
        wait(NULL); // Sol çocuğun bitmesini bekle
        wait(NULL); // Sağ çocuğun bitmesini bekle

        break;
    }
    exit(0);
}

// ------------------------------------------------------------
//                    getcmd()
// ------------------------------------------------------------
int getcmd(char *buf, int nbuf)
{
    if(isatty(fileno(stdin)))
        fprintf(stdout, "$ ");

    memset(buf, 0, nbuf);

    if(fgets(buf, nbuf, stdin) == 0)
        return -1;

    return 0;
}

// ------------------------------------------------------------
//                       main()
// ------------------------------------------------------------
// ******** SORU 4 ÇÖZÜMÜ BU FONKSİYONDA YAPILACAK   ********

int main(void)
{
    static char buf[MAXLINE];
    int r;

    while(1){

        if(getcmd(buf, sizeof(buf)) < 0)
            break;

        // newline temizleme
        size_t L = strlen(buf);
        if(L > 0 && buf[L-1] == '\n')
            buf[L-1] = 0;

        // boş satır atla
        int empty = 1;
        for(size_t i=0; i<strlen(buf); i++){
            if(buf[i] != ' ' && buf[i] != '\t' && buf[i] != '\r'){
                empty = 0;
                break;
            }
        }
        if(empty) continue;

        // builtin cd
        if(strncmp(buf, "cd ", 3) == 0){
            if(chdir(buf+3) < 0)
                fprintf(stderr, "cannot cd %s\n", buf+3);
            continue;
        }

        // önce parse edilir (arka plan olup olmadığı anlaşılacak)
        struct cmd *cmd = parsecmd(buf);
        int background = 0;

        // ----------------------------------------------------------------------
        // SORU 4: Arka plan (&) komut kontrolü için yazılacak kodlar buraya ...
        // ----------------------------------------------------------------------
        
        // Komutun tipi 'arka plan' ise
        if(cmd->type == '&'){
            background = 1;
            // 'cmd' değişkenini, arka plan yapısının içindeki
            // asıl komutla güncelle
            cmd = ((struct backcmd*)cmd)->cmd; 
        }

        pid_t pid = fork1();

        if(pid == 0){
            runcmd(cmd);
            exit(0);
        } else {
            if(!background){
                waitpid(pid, &r, 0);
            } else {
                // Not: Online-GDB terminali PID'yi hemen
                // göstermeyebilir, ancak kod mantığı doğrudur.
                printf("[BG %d]\n", pid);
            }
        }
    }

    return 0;
}


// ------------------------------------------------------------
//              ALT KISIM: DEĞİŞTİRMEYİNİZ !
// ------------------------------------------------------------

int fork1(void)
{
    int pid = fork();
    if(pid < 0){ perror("fork"); exit(1); }
    return pid;
}

struct cmd* execcmd(void)
{
    struct execcmd *cmd = malloc(sizeof(*cmd));
    memset(cmd,0,sizeof(*cmd));
    cmd->type = ' ';
    return (struct cmd*)cmd;
}

struct cmd* redircmd(struct cmd *subcmd, char *file, int type)
{
    struct redircmd *cmd = malloc(sizeof(*cmd));
    memset(cmd,0,sizeof(*cmd));

    cmd->type = type;
    cmd->cmd  = subcmd;
    cmd->file = file;

    if(type == '<'){ cmd->mode = O_RDONLY; cmd->fd = 0; }
    else if(type == '>'){ cmd->mode = O_WRONLY|O_CREAT|O_TRUNC; cmd->fd = 1; }
    else { cmd->mode = O_WRONLY|O_CREAT|O_TRUNC; cmd->fd = 2; }

    return (struct cmd*)cmd;
}

struct cmd* pipecmd(struct cmd *left, struct cmd *right)
{
    struct pipecmd *cmd = malloc(sizeof(*cmd));
    memset(cmd,0,sizeof(*cmd));
    cmd->type = '|';
    cmd->left = left;
    cmd->right = right;
    return (struct cmd*)cmd;
}

struct cmd* backcmd(struct cmd *subcmd)
{
    struct backcmd *cmd = malloc(sizeof(*cmd));
    memset(cmd,0,sizeof(*cmd));
    cmd->type = '&';
    cmd->cmd = subcmd;
    return (struct cmd*)cmd;
}

char whitespace[] = " \t\r\n\v";
char symbols[] = "<|>&";

int gettoken(char **ps, char *es, char **q, char **eq)
{
    char *s = *ps;

    while(s < es && strchr(whitespace,*s)) s++;
    if(q) *q = s;

    if(s >= es){ *ps = s; return 0; }

    if(*s=='2' && s+1<es && *(s+1)=='>'){ 
        if(eq) *eq = s+2;
        s+=2;
        while(s<es && strchr(whitespace,*s)) s++;
        *ps = s;
        return '2';
    }

    int ret = *s;
    switch(*s){
        case '|': case '<': case '>': case '&':
            s++; break;
        default:
            ret = 'a';
            while(s<es && !strchr(whitespace,*s) && !strchr(symbols,*s)) s++;
            break;
    }

    if(eq) *eq = s;

    while(s<es && strchr(whitespace,*s)) s++;
    *ps = s;

    return ret;
}

int peek(char **ps, char *es, char *toks)
{
    char *s = *ps;

    while(s<es && strchr(whitespace,*s)) s++;
    *ps = s;

    if(s >= es) return 0;

    if(*s=='2' && s+1<es && *(s+1)=='>')
        return strchr(toks,'2') != NULL;

    return *s && strchr(toks,*s);
}

char *mkcopy(char *s, char *es)
{
    int n = es - s;
    char *c = malloc(n+1);
    strncpy(c,s,n);
    c[n] = 0;
    return c;
}

struct cmd *parseline(char**,char*);
struct cmd *parsepipe(char**,char*);
struct cmd *parseexec(char**,char*);
struct cmd *parseredirs(struct cmd*,char**,char*);

struct cmd* parsecmd(char *s)
{
    char *es = s + strlen(s);
    struct cmd *cmd = parseline(&s, es);

    while(s<es && strchr(whitespace,*s)) s++;

    if(s != es){
        fprintf(stderr,"leftovers: %s\n",s);
        exit(1);
    }

    return cmd;
}

struct cmd* parseline(char **ps, char *es)
{
    struct cmd *cmd = parsepipe(ps, es);

    if(peek(ps, es, "&")){
        gettoken(ps, es, 0,0);
        cmd = backcmd(cmd);
    }

    return cmd;
}

struct cmd* parsepipe(char **ps, char *es)
{
    struct cmd *cmd = parseexec(ps, es);

    while(peek(ps, es, "|")){
        gettoken(ps, es, 0,0);
        cmd = pipecmd(cmd, parseexec(ps, es));
    }

    return cmd;
}

struct cmd* parseredirs(struct cmd *cmd, char **ps, char *es)
{
    int tok;
    char *q,*eq;

    while(peek(ps, es, "<>2")){
        tok = gettoken(ps, es, 0,0);

        if(gettoken(ps, es, &q, &eq) != 'a'){
            fprintf(stderr,"missing file for redirection\n");
            exit(1);
        }

        if(tok=='<') cmd = redircmd(cmd, mkcopy(q,eq), '<');
        else if(tok=='>') cmd = redircmd(cmd, mkcopy(q,eq), '>');
        else if(tok=='2') cmd = redircmd(cmd, mkcopy(q,eq), '2');
    }

    return cmd;
}

struct cmd* parseexec(char **ps, char *es)
{
    char *q,*eq;
    int tok, argc=0;

    struct execcmd *cmd = (struct execcmd*)execcmd();
    struct cmd *ret = (struct cmd*)cmd;

    ret = parseredirs(ret, ps, es);

    while(!peek(ps, es, "|&")){
        tok = gettoken(ps, es, &q, &eq);

        if(tok==0) break;
        if(tok!='a'){ fprintf(stderr,"syntax error\n"); exit(1); }

        cmd->argv[argc++] = mkcopy(q,eq);

        ret = parseredirs(ret, ps, es);

        if(argc >= MAXARGS){
            fprintf(stderr,"too many args\n");
            exit(1);
        }
    }

    cmd->argv[argc] = 0;
    return ret;
}
