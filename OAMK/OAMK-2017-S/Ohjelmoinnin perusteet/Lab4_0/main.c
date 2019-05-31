#include <stdio.h>

float tehtava1SyotaDesimaali()
{
    float result;
    scanf("%f", &result);
    return result;
}

int tehtava1SyotaKokonaisluku()
{
    int result;
    scanf("%i", &result);
    return result;
}

int tehtava1main()
{
    printf("anna desimaali luku\n");
    float f = tehtava1SyotaDesimaali();
    printf("anna kokonais luku\n");
    int i = tehtava1SyotaKokonaisluku();
    printf("%f %i\n", f, i);
    return 0;
}

float tehtava2YmpyraLaskenta(float sade)
{
    return sade * 2.0f * 3.14159265359f;
}

float tehtava2nelikulmioLaskenta(float x, float y)
{
    return (x * 2.0f) + (y * 2.0f);
}

void tehtava2main()
{
    for(int x;;)
    {
        printf("Mita lasketaan? 1 = ympyra, 2 = suorakulmion keha?\n");
        x = tehtava1SyotaKokonaisluku();
        if (x == 1)
        {
            printf("Anna sade\n");
            float y = tehtava1SyotaDesimaali();
            printf("keha on %f\n", tehtava2YmpyraLaskenta(y));
        }
        else if (x == 2)
        {
            printf("Anna leveys\n");
            float y = tehtava1SyotaDesimaali();
            printf("Anna korkeus\n");
            float z = tehtava1SyotaDesimaali();
            printf("keha on %f\n", tehtava2nelikulmioLaskenta(y, z));
        }
        else
            printf("%i ei ole sopiva arvo\n", x);
    }
}

int main()
{
    tehtava2main();
    return 0;
}
