#include <stdio.h>

void Lab3Tehtava1()
{
    printf("KUINKA MONTA HENKILOA\n");
    int n;
    scanf("%i", &n);
    int s = 0;
    for (int x, i = 0; i != n; ++i)
    {
        printf("KUINKA VANHA HENKILO\n");
        scanf("%i", &x);
        s += x;
    }
    printf("PERHEEN KESKI-IKA ON %i\n", s / n);
    return;
}

void Lab3Tehtava2()
{
    for(int x;;)
    {
        printf("Mita lasketaan? 1 = ympyra, 2 = suorakulmion keha?\n");
        scanf("%i", &x);
        if (x == 1)
        {
            printf("Anna sade\n");
            float y;
            scanf("%f", &y);
            printf("keha on %f\n", y * 2 * 3.14159265359);
        }
        else if (x == 2)
        {
            float y;
            float z;
            printf("Anna leveys\n");
            scanf("%f", &y);
            printf("Anna korkeus\n");
            scanf("%f", &z);
            printf("keha on %f\n", (y * 2) + (z * 2));
        }
        else
            printf("%i ei ole sopiva arvo\n", x);
    }
}

void Lab3Tehtava3B()
{

    int t = 1000;
    int n = -1;
    while (n < 0 || n > t)
    {
        printf("Anna nostettavan rahan maara euroina\n");
        scanf("%i", &n);
        if (n < 0)
           printf("%i euroa ei kelpaa nostettavaksi summaksi\n", n);
        else if (n > t)
            printf("%i euroa on liian suuri nostatavaksi. Tililla on vain %i euroa\n", n, t);
    }
    printf("rahaa nostetiin %i euroa. Tlille jai %i euroa\n", n, t - n);
}

void Jotain()
{


}

int main()
{
    Lab3Tehtava3B();
    return 0;
}
