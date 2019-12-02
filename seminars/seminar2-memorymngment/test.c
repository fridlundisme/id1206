#include "dlmall.h"
#include <stdio.h>

void main(void)
{
    
    init();
    print_flist();
    struct head *hej = dalloc(100);
    struct head *vad = dalloc(200);
    struct head *nej = dalloc(3000);
    print_flist();
    dfree(hej);
    dfree(vad);
    struct head *next = dalloc(4500);
    dfree(next);
    print_flist();
    dfree(nej);
    struct head *vasd = dalloc(5000);
    struct head *vsasd = dalloc(6000);
    struct head *vadsd = dalloc(7000);
    struct head *vasdh = dalloc(10000);
    struct head *vasfd = dalloc(8000);
    print_flist();
    struct head *vasdhg = dalloc(9000);
    struct head *vasdj = dalloc(2000);
    struct head *vasdjg = dalloc(4000);
    dfree(vasdhg);
    dfree(vasfd);
    print_flist();
    terminate();
}