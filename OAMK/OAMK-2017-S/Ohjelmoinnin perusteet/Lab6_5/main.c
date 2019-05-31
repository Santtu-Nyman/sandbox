#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAKSIMI_NIMEN_PITUUS 256
#define LISTAN_PITUUS 256

typedef struct pokemon
{
    char nimi[MAKSIMI_NIMEN_PITUUS];
    unsigned int lvl;
    unsigned int cp;
} pokemon;

void lueTeksti(char* teksti, unsigned int maksimiPituus)
{
    if (!maksimiPituus)
        return;
    teksti[maksimiPituus - 1] = 0;
    scanf("%s", teksti);
    if (teksti[maksimiPituus - 1])
        abort();
}

int main()
{
    char nimi[MAKSIMI_NIMEN_PITUUS];
    pokemon lista[LISTAN_PITUUS];
    unsigned int listaaKaytossa = 0;
    unsigned int komento;
    unsigned int loytyi;
    do
    {
        printf("Valitse seuraavista vaihtoehdoista:\n0. Lisaa Pokemon\n1. Tulosta kaikki Pokemonit\n2. Etsi Pokemoneja\n3. Poista kaikki Pokemonit\n4. Poista pokemoni\n5. Lopeta\n");
        scanf("%u", &komento);
        switch (komento)
        {
            case 0 :
                if (listaaKaytossa != LISTAN_PITUUS)
                {
                    printf("Anna pokemonin nimi: ");
                    lueTeksti(lista[listaaKaytossa].nimi, MAKSIMI_NIMEN_PITUUS);
                    printf("Anna CP: ");
                    scanf("%u", &lista[listaaKaytossa].cp);
                    printf("Anna level: ");
                    scanf("%u", &lista[listaaKaytossa].lvl);
                    ++listaaKaytossa;
                }
                else
                    printf("lista tayna ei mahdu enempaa\n");
                break;
            case 1 :
                for (pokemon* i = lista, * e = i + listaaKaytossa; i != e; ++i)
                    printf("***POKEMON:\nNIMI: %s\nCP: %u\nLEVEL %u\n", i->nimi, i->cp, i->lvl);
                break;
            case 2 :
                printf("Anna etsittava nimi: ");
                lueTeksti(nimi, MAKSIMI_NIMEN_PITUUS);
                loytyi = 0;
                for (pokemon* i = lista, * e = i + listaaKaytossa; !loytyi && i != e; ++i)
                    if (!strcmp(nimi, i->nimi))
                    {
                        printf("***POKEMON:\nNIMI: %s\nCP: %u\nLEVEL %u\n", i->nimi, i->cp, i->lvl);
                        loytyi = 1;
                    }
                if (!loytyi)
                    printf("ei loytyny\n");
                break;
            case 3 :
                listaaKaytossa = 0;
                break;
            case 4 :
                printf("Anna poistettava nimi: ");
                lueTeksti(nimi, MAKSIMI_NIMEN_PITUUS);
                loytyi = 0;
                for (pokemon* i = lista, * e = i + listaaKaytossa; !loytyi && i != e; ++i)
                    if (!strcmp(nimi, i->nimi))
                    {
                        if (i + 1 != e)
                            for (; i + 1 != e; ++i)
                                memcpy(i, i + 1, sizeof(pokemon));
                        --listaaKaytossa;
                        loytyi = 1;
                    }
                if (!loytyi)
                    printf("ei loytyny\n");
                break;
            case 5 :
                break;
            default :
                printf("valinta ei kalpaa. ");
                break;
        }

    } while (komento != 5);
    return 0;
}
