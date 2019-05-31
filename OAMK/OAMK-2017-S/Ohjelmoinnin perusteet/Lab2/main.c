#include <stdio.h>
#include <stdlib.h>

void Tehtava_1_1 ()
{
    int i;
    printf("Anna kokonaisluku\n");
    scanf("%d", &i);
    printf("%i on %s\n", i, i ? i < 0 ? "negatiivinen" : "positiivinen" : "neuttraali");
}

void Tehtava_1_2 ()
{
    int i[2];
    printf("Anna kaksi kokonaislukua\n");
    scanf("%d %d", &i[0], &i[1]);
    printf(i[0] % i[1] ? "Ensimmainen luku ei ole jaollinen toisella\n" : "Ensimmainen luku on jaollinen toisella\n");
}

void Tehtava_1_3 ()
{
    for (int s = 17, i = ~s; i != s;)
    {
        printf("Anna salanumero\n");
        scanf("%d", &i);
        printf(i == s ? "jee jee onnittelut numerosi on oikein\n" : "numero vaarin ");
    }
}

void Tehtava_1_4 ()
{
    char nimi[128];
    nimi[127] = 0;
    printf("Anna nimesi\n");
    scanf("%s", nimi);
    if (nimi[127])
    {
        printf("virhe: puskurin yli kirjoitettii\n");
        abort();
    }
    printf("Nimesi on %s\n", nimi);
}

void Tehtava_2 ()
{
    int i[2];
    printf("Anna kaksi kokonaislukua\n");
    scanf("%d %d", &i[0], &i[1]);
    printf("lokujen summa on %d\n", i[0] + i[1]);
}

void Tehtava_3 ()
{
    int i[2];
    printf("Anna kaksi kokonaislukua\n");
    scanf("%d %d", &i[0], &i[1]);
    if (i[0] > i[1])
    {
        int valiaikainen = i[0];
        i[0] = i[1];
        i[1] = valiaikainen;
    }
    printf("%d %d\n", i[0], i[1]);
}

void Tehtava_4()
{
    const float pii = 3.14159265359f;
    unsigned long ulongMuutuja = 0xFFFFFFFF;
    printf("pii on noin %f se ei ole kokonausluku\n", pii);
    printf("32 bittisen luvun maksimi arvo %ul se on %X hexadesimaalina\n", ulongMuutuja, ulongMuutuja);
    printf("laskuri viidesta miinus viiteen\n");
    for (int i = 5; i != -6; --i)
        printf("%i ", i);
    printf("\n");
};

void Tehtava_5 ()
{
    const int arvosanaLista[7] = { 0, 7, 11, 14, 18, 21, 24 };
    int pisteMaara;
    printf("Anna opiskelijan pistenaara\n");
    scanf("%d", &pisteMaara);
    if (pisteMaara < *arvosanaLista || pisteMaara > arvosanaLista[sizeof(arvosanaLista) / sizeof(int) - 1])
    {
        printf("virheellinen piste naara\n");
        return;
    }
    int arvosana = 0;
    while (pisteMaara > arvosanaLista[arvosana + 1])
        ++arvosana;
    if (arvosana)
        printf("Arvosana on %d\n", arvosana);
    else
        printf("Koe ei mennet lapi\n");
}

void Tehtava_6 ()
{
    printf("Anna kokonaislukuja ja lopeta lukujen antamenen luvulla 0\n");
    int i[2] = { 0, 0 };
    do
    {
        scanf("%d", &i[1]);
        i[0] += i[1];
    } while (i[1]);
    printf("Lukujen summa on %d\n", i[0]);
}

int main()
{
    for (;;)
     Tehtava_5();
    return 0;
};
