#include <stdio.h>

void lab6TehtavaA()
{
    int i[2];
    printf("Anna kaksi numeroa\n");
    scanf("%i", i);
    scanf("%i", i + 1);
    printf(i[0] == i[1] ? "numerot ovat samoja\n" : "numerot eivat ole samoja\n");
}

void lab6TehtavaBSort(int length, int* data)
{
    if (!length)
        return;
    for (int index = 0; index != length - 1;)
    {
        int t = data[index];
        if (t > data[index + 1])
        {
            data[index] = data[index + 1];
            data[index + 1] = t;
            if (index)
                --index;
        }
        else
            ++index;
    }
}

void lab6TehtavaB()
{
    int i[3];
    printf("Syota kolme momeroa\n");
    for (int l = 3; l--;)
        scanf("%i", i + l);
    lab6TehtavaBSort(3, i);
    for (int l = 0; l != 3; ++l)
        printf("%i ", i[l]);
    printf("\n");
}

void lab6TehtavaC()
{
    unsigned int bruttopalka;
    unsigned int veroprosenti;
    unsigned int nettopalka;
    printf("Syota bruttopalka\n");
    scanf("%i", &bruttopalka);
    printf("Syota veroprosenti\n");
    scanf("%i", &veroprosenti);
    bruttopalka <<= 10;
    veroprosenti <<= 10;
    veroprosenti /= 100;
    nettopalka = bruttopalka - ((bruttopalka * veroprosenti) >> 10);
    printf("Nettopalka on %u\n", nettopalka >> 10);
}

void Lab6TehtavaD()
{
    for (int i = 0; i++ != 100;)
        printf("%i\n", i);
}

void Lab6TehtavaE()
{
    int d[5] = { 5, 3, 5, 6, 9 };
    int r = 0;
    for (int* i = d, * e = i + 5; i != e; ++i)
        r += *i;
    printf("%i\n", r);
}

void Lab6TehtavaF()
{
    int d[5];
    printf("Syota viisi numeroa\n");
    for (int* i = d, * e = i + 5; i != e; ++i)
        scanf("%i", i);
    int r = 0;
    for (int t, * i = d, * e = i + 5; i != e; ++i)
    {
        t = *i;
        r += t;
        if (i + 1!= e)
            printf("%i + ", t);
        else
            printf("%i = %i\n", t, r);
    }
}

void Lab6TehtavaG()
{
    printf("Syota celsius lampo arvo\n");
    float c;
    scanf("%f", &c);
    float f = c * 1.8f + 32.0f;
    printf("fahrenheitteina %f\n", f);
}

void lab6TehtavaH()
{
    for (float c = -50.0f; c < 51.0f; ++c)
        printf ("%f Celsius = %f Fahrenheit\n", c, c * 1.8f + 32.0f);
}

#include <string.h>

typedef struct valuutanInfo
{
    float arvoEuroonVerrattuna;
    char shortName[4];
    char yksikko[256];
} valuutanInfo;

void lab6TehtavaI()
{
    valuutanInfo valuutat[3];
    valuutat[0].arvoEuroonVerrattuna = 1.0f;
    memcpy(valuutat[0].shortName, "EUR", 4);
    memcpy(valuutat[0].yksikko, "euroa", 6);
    valuutat[1].arvoEuroonVerrattuna = 1.179f;
    memcpy(valuutat[1].shortName, "USD", 4);
    memcpy(valuutat[1].yksikko, "dollaria", 9);
    valuutat[2].arvoEuroonVerrattuna = 0.877f;
    memcpy(valuutat[2].shortName, "GDP", 4);
    memcpy(valuutat[2].yksikko, "puntaa", 7);
    int sisaantuloValuutta;
    for (int l = 1; l;)
    {
        printf("valitse valuuttakurssin josta haluat knvertoida\n");
        for (int i = 0; i != sizeof(valuutat) / sizeof(valuutanInfo); ++i)
             printf("%i, %s\n", i, valuutat[i].shortName);
        scanf("%i", &sisaantuloValuutta);
        if (sisaantuloValuutta > -1 && sisaantuloValuutta < (int)(sizeof(valuutat) / sizeof(valuutanInfo)))
            l = 0;
        else
            printf("valinta ei kelpaa ");
    }
    printf("anna rahan maara\n");
    float sisaantuloRaha;
    scanf("%f", &sisaantuloRaha);
    sisaantuloRaha /= valuutat[sisaantuloValuutta].arvoEuroonVerrattuna;
    int ulostuloValuutta;
    for (int l = 1; l;)
    {
        printf("valitse valuuttakurssin johon haluat knvertoida\n");
        for (int i = 0; i != sizeof(valuutat) / sizeof(valuutanInfo); ++i)
             printf("%i, %s\n", i, valuutat[i].shortName);
        scanf("%i", &ulostuloValuutta);
        if (ulostuloValuutta > -1 && ulostuloValuutta < (int)(sizeof(valuutat) / sizeof(valuutanInfo)))
            l = 0;
        else
            printf("valinta ei kelpaa ");
    }
    printf("%f %s = %f %s\n",
           sisaantuloRaha * valuutat[sisaantuloValuutta].arvoEuroonVerrattuna, valuutat[sisaantuloValuutta].shortName,
           sisaantuloRaha * valuutat[ulostuloValuutta].arvoEuroonVerrattuna, valuutat[ulostuloValuutta].shortName);
}

void lab6TehtavaJ()
{
    printf("hakkaa enteria niin saat pisteita");
    for (unsigned int i = 0;; ++i)
    {
        for (char t = 0; t != '\n';)
            t = getchar();
        printf("pistaasi %u\n", i);
    }
}

#include <stdlib.h>
#include <time.h>

void lab6TehtavaL()
{
    printf("satunnaisia numeroja ");
    int d[8];
    srand((unsigned int)lab6TehtavaL ^ (unsigned int)time(0));
    for (int* i = d, x, * e = i + 8; i != e; ++i)
    {
        x = 0;
        while (!x)
        {
            x = (rand() % 40) + 1;
            for (int* s = d; x && s != i; ++s)
                if (x == *s)
                    x = 0;
        }
        *i = x;
    }
    lab6TehtavaBSort(8, d);
    for (int* i = d, * e = i + 8; i != e; ++i)
        printf(i + 1 != e ? "%i " : "%i\n", *i);
}

#include "lab6tehtavak.h"

int main()
{
    lab6TehtavaL();
    return 0;
}
