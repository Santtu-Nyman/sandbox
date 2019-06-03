#include <stdio.h>
#include <stdlib.h>

#define HENKILO_LISTAN_PITUUS 16
#define MAKSIMI_NIMEN_PITUUS 255

typedef struct henkilo_t
{
    unsigned int onOlemassa;
    unsigned int henkilotunnus;
    unsigned int ika;
    unsigned int palkka;
    char nimi[MAKSIMI_NIMEN_PITUUS];

} henkilo_t;

void annaHenkilonTiedot(henkilo_t* henkilo)
{
    henkilo->onOlemassa = 1;
    printf("Anna henkilon henkilotunnus\n");
    scanf("%u", &henkilo->henkilotunnus);
    printf("Anna hekilon ika\n");
    scanf("%u", &henkilo->ika);
    printf("Anna hekilon palkka\n");
    scanf("%u", &henkilo->palkka);
    henkilo->nimi[MAKSIMI_NIMEN_PITUUS - 1] = 0;
    printf("Anna hekilon nimi\n");
    scanf("%s", henkilo->nimi);
    if (henkilo->nimi[MAKSIMI_NIMEN_PITUUS - 1])
        abort();
}

void tulostaHenkilonTiedot(henkilo_t* henkilo)
{
    if(!henkilo->onOlemassa)
        return;
    printf("Henkilo\n");
    printf("Nimi %s\n", henkilo->nimi);
    printf("Palkka %u\n", henkilo->palkka);
    printf("Ika %u\n", henkilo->ika);
    printf("Henkilotunnus %u\n", henkilo->henkilotunnus);
}

henkilo_t* etsiVapaaPaikka(henkilo_t* lista)
{
    for (henkilo_t* e = lista + HENKILO_LISTAN_PITUUS; lista != e; ++lista)
        if (!lista->onOlemassa)
            return lista;
    return 0;
}

unsigned int vertaaTeksteja(const char* teksti0, const char* teksti1)
{
    while (*teksti0 && *teksti0 == *teksti1)
    {
        ++teksti0;
        ++teksti1;
    }
    return *teksti0 == *teksti1;
}

void poistaHenkilo(henkilo_t* lista)
{
    char nimi[MAKSIMI_NIMEN_PITUUS];
    printf("Anna poistettavan hekilon nimi\n");
    nimi[MAKSIMI_NIMEN_PITUUS - 1] = 0;
    scanf("%s", nimi);
    if (nimi[MAKSIMI_NIMEN_PITUUS - 1])
        abort();
    for (henkilo_t* e = lista + HENKILO_LISTAN_PITUUS; lista != e; ++lista)
        if (vertaaTeksteja(nimi, lista->nimi))
        {
            lista->onOlemassa = 0;
            return;
        }
    printf("henkiloa ei loytynyt\n");
}

int main()
{
    henkilo_t henkiloLista[HENKILO_LISTAN_PITUUS];
    for (henkilo_t* l = henkiloLista,* e = l + HENKILO_LISTAN_PITUUS; l != e; ++l)
        l->onOlemassa = 0;
    unsigned int komento;
    do
    {
        printf ("Valitse seuraavista vaihtoehdoista\n1. Lisaa henkilo\n2. Tulosta henkilot\n3. Poista henkilo\n4. Poista kaikka henkilot\n5 Poistu\n");
        scanf("%u", &komento);
        if (komento == 1)
        {
            henkilo_t* paikka = etsiVapaaPaikka(henkiloLista);
            if (paikka)
                annaHenkilonTiedot(paikka);
            else
                 printf("Ei tilaa\n");
        }
        else if (komento == 2)
            for (henkilo_t* l = henkiloLista,* e = l + HENKILO_LISTAN_PITUUS; l != e; ++l)
                tulostaHenkilonTiedot(l);
        else if (komento == 3)
            poistaHenkilo(henkiloLista);
        else if (komento == 4)
            for (henkilo_t* l = henkiloLista,* e = l + HENKILO_LISTAN_PITUUS; l != e; ++l)
                l->onOlemassa = 0;
        else if (komento != 5)
            printf("Valinta ei kelpaa ");
    } while (komento != 5);
    return 0;
}
