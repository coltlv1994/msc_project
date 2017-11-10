#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <math.h>

int main(void) {
    int index = 0;
    int latency[500];
    char* inp;
    double sum = 0;
    double sum2 = 0;
    inp = (char*)malloc(sizeof(char)*10);
    FILE* f = fopen("/home/colt/Downloads/ns-allinone-3.26/ns-3.26/output_detail","r");
    if(!f)
        return 1;
    
    char c;
    int i = 0;
    while((c = fgetc(f))!=EOF) {
        if(c=='l') {
            if((c = fgetc(f))=='a') {
                if((c = fgetc(f))=='t') {
                    if((c = fgetc(f))=='o') {
                        if((c = fgetc(f))=='p') {
                            fgetc(f);
                            // printf("all match\n");
                            while(isdigit(c=fgetc(f))) {
                                inp[i++] = c;
                                inp[i] = '\0';
                            }
                            // printf("%s\n",inp);
                            latency[index++] = atoi(inp);
                            // printf("%d\n",index);
                            i = 0;
                        }
                    }
                }
            }
        }
    }

    for(int j = 0; j < index; j++) {
        sum += (double)latency[j];
        sum2 +=(double)latency[j]*(double)latency[j];
    }
    printf("%d\n%f\n%f\n",index,sum/(double)index,sqrt(sum*sum-sum2));
    free(inp);
    fclose(f);
    return 0;
}