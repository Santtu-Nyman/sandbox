#include <stdio.h>
#include "tx.h"

void tehtava3main()
{
    for(int x;;)
    {
        printf("Mita lasketaan? 1 = ympyra, 2 = suorakulmion keha?\n");
        x = tehtava3SyotaKokonaisluku();
        if (x == 1)
        {
            printf("Anna sade\n");
            float y = tehtava3SyotaDesimaali();
            printf("keha on %f\n", tehtava3YmpyraLaskenta(y));
        }
        else if (x == 2)
        {
            printf("Anna leveys\n");
            float y = tehtava3SyotaDesimaali();
            printf("Anna korkeus\n");
            float z = tehtava3SyotaDesimaali();
            printf("keha on %f\n", tehtava3nelikulmioLaskenta(y, z));
        }
        else
            printf("%i ei ole sopiva arvo\n", x);
    }
}

void tehtava4main()
{
    printf("anna vuosiluku\n");
    unsigned int vuosi;
    scanf("%u", &vuosi);
    printf("vuosi %u %s karkaus vuosi\n", vuosi, tehtava4OnkoKarkausVuosi(vuosi) ? "on" : "ei ole");
}

int main()
{
    tehtava4main();
    return 0;
}
