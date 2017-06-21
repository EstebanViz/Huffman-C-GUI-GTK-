#include <stdlib.h>
#include <gtk/gtk.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<malloc.h>
#include<time.h>

struct nodo
{
    struct nodo *der,*izq,*arr;  /* forma el nodo */
    int cuenta;                  /* apariciones del carácter */
    char bit;                    /* 0 o 1 */
    unsigned char karacter;      /* el carácter (para la descompresión */
    char *codigo;                /* cadena de ceros y unos con la codificación */
    char nbits;                  /* me apunto el numero de bits que codifican el carácter */
} HOJAS[256],*TELAR[256],*MENOR,*SEGUNDO;

int NSIMB=0,nsimb;
FILE *f,*g;
int NBYTES=0;

/*--------------------------------
preparar las hojas
--------------------------------*/
int preparar_hojas(char *archivo)
{
    int j;
    for(j=0; j<256; ++j)
    {
        HOJAS[j].der=HOJAS[j].izq=HOJAS[j].arr=NULL;
        HOJAS[j].cuenta=0;
        HOJAS[j].karacter=j;
        HOJAS[j].codigo=NULL;
    }
    if ((f=fopen(archivo,"rb"))!=NULL)
    {
        while ((j=fgetc(f))!=EOF)
        {
            ++HOJAS[j].cuenta;
            ++NBYTES;
        }
        fclose(f);
    }
    else
    {
        return(1);
    }
    for(j=0; j<256; ++j)
    {
        if (HOJAS[j].cuenta!=0)
            ++NSIMB;
    }
    nsimb=NSIMB;
    return(0);
}

/*--------------------------------
preparar telar
--------------------------------*/
void preparar_telar()
{
    int j;
    for(j=0; j<256; ++j)
    {
        TELAR[j]=&(HOJAS[j]);
    }
    return;
}

/*--------------------------------
tejer el árbol
--------------------------------*/
void tejer()
{
    int menor=-1;     /* guarda indice */
    int segundo=-1;   /* guarda indice */
    int temporal;     /* guarda cuenta */
    int j;
    struct nodo *P;   /* nuevo nodo */

    if (nsimb==1) return;

    /* buscar menor valor */
    for(j=0; j<256; ++j)
    {
        if (TELAR[j]==NULL) continue;
        if (TELAR[j]->cuenta==0) continue;
        if (menor==-1)
        {
            menor=j;
            temporal=TELAR[j]->cuenta;
        }
        else
        {
            if (TELAR[j]->cuenta<temporal)
            {
                menor=j;
                temporal=TELAR[j]->cuenta;
            }
        }
    }

    /* buscar segundo menor */
    for(j=0; j<256; ++j)
    {
        if (TELAR[j]==NULL) continue;
        if (TELAR[j]->cuenta==0) continue;
        if (j==menor) continue;
        if (segundo==-1)
        {
            segundo=j;
            temporal=TELAR[j]->cuenta;
        }
        else
        {
            if (TELAR[j]->cuenta<temporal)
            {
                segundo=j;
                temporal=TELAR[j]->cuenta;
            }
        }
    }

    /* tejer un nuevo nodo */
    P=(struct nodo *)malloc(sizeof(struct nodo));
    TELAR[menor]->arr=P;
    TELAR[segundo]->arr=P;
    P->izq=TELAR[menor];
    P->der=TELAR[segundo];
    P->arr=NULL;
    TELAR[menor]->bit=0;
    TELAR[segundo]->bit=1;
    P->cuenta=TELAR[menor]->cuenta+TELAR[segundo]->cuenta;
    TELAR[menor]=NULL;
    TELAR[segundo]=P;
    --nsimb;

    /* sigue tejiendo hasta que sólo quede un nodo */
    tejer();
}

/*--------------------------------
Una vez construido el árbol, puedo codificar
cada carácter. Para eso recorro desde la hoja
a la raíz, apunto 0 o 1 en una pila y luego
paso la pila a una cadena. Un 2 determina el
fin de la cadena.
--------------------------------*/
void codificar()
{
    char pila[64];
    char tope;
    int j;
    char *w;
    struct nodo *P;
    for(j=0; j<256; ++j)
    {
        if (HOJAS[j].cuenta==0) continue;
        P=(struct nodo *)(&(HOJAS[j]));
        tope=0;
        while (P->arr!=NULL)
        {
            pila[tope]=P->bit;
            ++tope;
            P=P->arr;
        }
        HOJAS[j].nbits=tope;
        HOJAS[j].codigo=(char *)malloc((tope+1)*sizeof(char));
        w=HOJAS[j].codigo;
        --tope;
        while (tope>-1)
        {
            *w=pila[tope];
            --tope;
            ++w;
        }
        *w=2;
    }
    return;
}


/*--------------------------------
debug. Imprime la info sobre cada
carácter, como número de apariciones
y cadena con que se codifica
--------------------------------*/
void debug()
{
    int j,k;
    char *w;
    int tam_comprimido=0;
    for(j=0; j<256; ++j)
    {
        if (HOJAS[j].cuenta==0) continue;
        tam_comprimido+=(HOJAS[j].cuenta*HOJAS[j].nbits);
        printf("%3d %6d ",j,HOJAS[j].cuenta);
        w=HOJAS[j].codigo;
        while (*w!=2)
        {
            printf("%c",48+(*w));
            ++w;
        }
        printf("\n");
    }
    printf("NSIMB: %d\n",NSIMB);
    printf("NBYTES: %d\n",NBYTES);
    printf("TAMAÑO COMPRIMIDO: %d\n",tam_comprimido/8+1);
    return;
}

/*--------------------------------
Escribe la cabecera del archivo de
destino. La cabecera contiene: el
número de bytes del archivo origen,
el número de caracteres distintos
en ese archivo y una lista de parejas
número de carácter-cuenta de ese
carácter. Eso es suficiente para la
descompresión
--------------------------------*/
int escribe_cabecera(char *destino)
{
    int j,k;
    FILE *g;

    char *p=(char *)(&NBYTES);
    if ((g=fopen(destino,"wb"))==NULL) return(1);
    for(j=0; j<4; ++j)
    {
        fputc(*p,g);
        ++p;
    }

    p=(char *)(&NSIMB);
    fputc(*p,g);

    for(j=0; j<256; ++j)
    {
        if (HOJAS[j].cuenta==0) continue;
        fputc(j,g);
        p=(char *)(&(HOJAS[j].cuenta));
        for(k=0; k<4; ++k)
        {
            fputc(*p,g);
            ++p;
        }
    }
    fclose(g);
    return(0);
}

/*--------------------------------
Una vez construido el árbol y codificado
cada carácter se puede proceder a la
compresión: se tomará carácter a carácter
del archivo origen y se usará la cadena
de codificación para ir escribiendo
bits en un buffer de un carácter, que
cada vez que quede lleno se pasará al
archivo de destino
--------------------------------*/
int comprimir(char *origen, char *destino)
{
    unsigned char d=0;
    int x;
    char nbit=0;
    char *p;

    if ((f=fopen(origen,"rb"))==NULL) return(1);
    if ((g=fopen(destino,"ab"))==NULL) return(2); /* ya esta la cabecera */

    while ((x=fgetc(f))!=EOF)
    {
        p=HOJAS[x].codigo;
        while (*p!=2)
        {
            if (nbit==8)
            {
                nbit=0;
                fputc(d,g);
                d=0;
            }
            else
            {
                if (*p==1)
                {
                    d|=(1<<nbit);
                }
                ++nbit;
                ++p;
            }
        }
    }
    fputc(d,g);
    fclose(f);
    fclose(g);
    return(0);
}

/*--------------------------------
Descomprime el archivo. El primer paso
es leer la cabecera, paso previo a la
descompresión. Recuerdo formato de
la cabecera:
NBYTES|NSIMB|(char,cuenta)*
--------------------------------*/
int descomprimir(char *origen, char *destino)
{
    char *p;
    int j,k,n,m;
    unsigned char x,nbit;
    struct nodo *P,*Q;

    if ((g=fopen(origen,"rb"))==NULL) return(1);
    if ((f=fopen(destino,"wb"))==NULL) return(2);

    /* leer NBYTES */
    p=(char *)(&n);
    for(j=0; j<4; ++j)
    {
        *p=(unsigned char)fgetc(g);
        ++p;
    }
    NBYTES=n;

    /* leer NSIMB */
    NSIMB=nsimb=fgetc(g);

    /* preparar las hojas */
    for(j=0; j<256; ++j)
    {
        HOJAS[j].cuenta=0;
        HOJAS[j].izq=HOJAS[j].der=HOJAS[j].arr=NULL;
        HOJAS[j].karacter=j;
    }
    for(j=0; j<NSIMB; ++j)
    {
        n=fgetc(g);
        p=(char *)(&m);
        for(k=0; k<4; ++k)
        {
            *p=(unsigned char)fgetc(g);
            ++p;
        }
        HOJAS[n].cuenta=m;
    }

    /* construyo el árbol */
    preparar_telar();
    tejer();

    /* apunto a la raíz del árbol */
    j=0;
    while (HOJAS[j].cuenta==0) ++j;
    P=(struct nodo *)(&(HOJAS[j]));
    while (P->arr!=NULL) P=P->arr;

    /* ahora ya se puede descomprimir */
    j=0;
    x=fgetc(g);
    nbit=0;
    Q=P;
    while(j<NBYTES)
    {
        if (Q->izq==NULL)
        {
            fputc(Q->karacter,f);
            Q=P;
            ++j;
        }
        else if (nbit==8)
        {
            x=fgetc(g);
            nbit=0;
        }
        else
        {
            if (x&(1<<nbit))
            {
                Q=Q->der;
            }
            else
            {
                Q=Q->izq;
            }
            ++nbit;
        }
    }
    fclose(f);
    fclose(g);
    return(0);
}

GtkWidget * Ventana1;  //Para gt

char texto[3500];   // guarda  el nombre del archivo que se selecciono
char texto2[3500];
long int tamano;
long int tamano2;
float total;
float total2;

GtkLabel *VarTam,*VarNombre,*VarTiempo;
char datos [50];

void mostrarDatos()
{

    GtkWidget *dialog;
    dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                    GTK_MESSAGE_INFO, GTK_BUTTONS_OK,"Dirección del archivo elegido: %s\n Tamaño del archivo: %i bytes  \n  Tiempo de ejecución: %f segundos ",texto,tamano,total);
    if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK)
    {
        gtk_widget_destroy(Ventana1);
        gtk_widget_destroy(dialog);
    }
    main();
}

void mostrarDatos2()
{
    GtkWidget *dialog;
    dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                    GTK_MESSAGE_INFO, GTK_BUTTONS_OK,"Dirección del archivo elegido: %s\n Tamaño del archivo: %i bytes \n Tiempo de ejecución: %f segundos ",texto2,tamano2,total2);
    if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK)
    {
        gtk_widget_destroy(Ventana1);
        gtk_widget_destroy(dialog);
    }
    main();



}

void descomprimirDatos()
{

    //printf("ENTRE AQUI");
    //printf("Direccion del texto: %s \n",texto2);
    descomprimir(texto2,"Descomprimir.txt");

}
void comprimirDatos()
{

    preparar_hojas(texto);
    preparar_telar();
    tejer();
    codificar();
    escribe_cabecera("Comprimido.txt");
    comprimir(texto,"Comprimido.txt");
}
void cerrar()
{

    GtkWidget *dialog;
    dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                    GTK_MESSAGE_INFO, GTK_BUTTONS_OK,"%s","Gracias");
    if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK)
    {
        gtk_widget_destroy(Ventana1);
    }
    exit(0);
    main();
}

void regresar()
{

    gtk_widget_destroy(Ventana1);
    main();

}


void abrir_archivo2()    //para el descomprimir
{
    GtkWidget *VentanaAbrir;
    GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;
    gint respuesta;
    GtkFileFilter *filtro, *filtro2;
    filtro = gtk_file_filter_new();
    filtro2 = gtk_file_filter_new();
    gtk_file_filter_set_name(filtro, "Archivos de Texto");
    gtk_file_filter_set_name(filtro2, "Todos los archivos");
    gtk_file_filter_add_pattern(filtro, "*.txt");
    gtk_file_filter_add_pattern(filtro2, "*");
    VentanaAbrir = gtk_file_chooser_dialog_new("Abrir Archivo", Ventana1, action, "Abrir", GTK_RESPONSE_ACCEPT,"Cancelar", GTK_RESPONSE_CANCEL, NULL);

    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(VentanaAbrir), filtro);
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(VentanaAbrir), filtro2);
    respuesta = gtk_dialog_run(GTK_DIALOG(VentanaAbrir));

    if(respuesta == GTK_RESPONSE_ACCEPT)
    {
        char *filename;
        GtkFileChooser *chooser = GTK_FILE_CHOOSER(VentanaAbrir);
        filename =  gtk_file_chooser_get_filename(chooser);

        g_print("DIRECCION DEL ARCHIVO: %s\n",filename);
        strcpy(texto2,filename);
        //printf("Direccion del archivo a comprimir: %s",texto2);
        FILE *fich;

        fich=fopen(texto2,"r");

        fseek(fich, 0L, SEEK_END);

        printf("test.c ocupa %d bytes\n", ftell(fich));

        tamano2 = ftell(fich);

        fclose(fich);
        clock_t inicio,fin;
        inicio = clock();
        /* Aqui va el codigo cuyo tiempo de ejecucion quieres medir */
        descomprimirDatos();
        fin = clock();
        // obtenemos y escribimos el tiempo en segundos
        printf("inicio: %i",inicio);
        printf("fin: %i",fin);
        float dividir = CLOCKS_PER_SEC;
        printf("DIVIDIR: %f ",dividir);
        total2 = (fin - inicio)/dividir;
        printf("Tiempo empleado: %f\n",total2);
        gtk_widget_destroy(VentanaAbrir);
        mostrarDatos2();
        regresar();

    }
    if(respuesta== GTK_RESPONSE_CANCEL)
    {

        gtk_widget_destroy(VentanaAbrir);
        main();

    }

}

void abrir_archivo()
{
    GtkWidget *VentanaAbrir;
    GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;
    gint respuesta;
    GtkFileFilter *filtro, *filtro2;
    filtro = gtk_file_filter_new();
    filtro2 = gtk_file_filter_new();
    gtk_file_filter_set_name(filtro, "Archivos de Texto");
    gtk_file_filter_set_name(filtro2, "Todos los archivos");
    gtk_file_filter_add_pattern(filtro, "*.txt");
    gtk_file_filter_add_pattern(filtro2, "*");
    VentanaAbrir = gtk_file_chooser_dialog_new("Abrir Archivo", Ventana1, action, "Abrir", GTK_RESPONSE_ACCEPT,"Cancelar", GTK_RESPONSE_CANCEL, NULL);

    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(VentanaAbrir), filtro);
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(VentanaAbrir), filtro2);
    respuesta = gtk_dialog_run(GTK_DIALOG(VentanaAbrir));
    tamano=0;

    if(respuesta == GTK_RESPONSE_ACCEPT)
    {
        char *filename;
        GtkFileChooser *chooser = GTK_FILE_CHOOSER(VentanaAbrir);
        filename =  gtk_file_chooser_get_filename(chooser);
        g_print("DIRECCION DEL ARCHIVO: %s\n",filename);
        strcpy(texto,filename);
        FILE *fich;
        fich=fopen(texto,"r");
        fseek(fich, 0L, SEEK_END);
        printf("test.c ocupa %d bytes\n", ftell(fich));
        tamano = ftell(fich);
        fclose(fich);

        clock_t inicio,fin;
        inicio = clock();
        /* Aqui va el codigo cuyo tiempo de ejecucion quieres medir */
        comprimirDatos();
        fin = clock();
        // obtenemos y escribimos el tiempo en segundos
        printf("inicio: %i",inicio);
        printf("fin: %i",fin);
        float dividir = CLOCKS_PER_SEC;
        printf("DIVIDIR: %f ",dividir);
        total = (fin - inicio)/dividir;
        printf("Tiempo empleado: %f\n",total);
        gtk_widget_destroy(VentanaAbrir);
        mostrarDatos();
        regresar();

    }
    if(respuesta== GTK_RESPONSE_CANCEL)
    {

        gtk_widget_destroy(VentanaAbrir);
        main();

    }

}

int main (int argc, char *argv[])
{

    gtk_init(&argc,&argv);
    GtkBuilder *constructor;
    // aqui se crear las variables para los widgets de Glade

    GtkWidget *Titulo,*Archivo,*Buscar,*Nombre,*Tamano,*TiempoEjecucion,*Codificar,*Decodificar,*Salir;

    constructor=gtk_builder_new();
    gtk_builder_add_from_file(constructor,"PantallaPrincipal.glade",NULL);


    Ventana1 = GTK_WIDGET(gtk_builder_get_object(constructor,"Ventana"));
    Titulo = GTK_WIDGET(gtk_builder_get_object(constructor,"Titulo"));
    Codificar = GTK_WIDGET(gtk_builder_get_object(constructor,"Codificar"));
    Decodificar = GTK_WIDGET(gtk_builder_get_object(constructor,"Decodificar"));
    Salir = GTK_WIDGET(gtk_builder_get_object(constructor,"Salir"));


    g_signal_connect(Codificar,"clicked",G_CALLBACK(abrir_archivo),NULL);
    g_signal_connect(Decodificar,"clicked",G_CALLBACK(abrir_archivo2),NULL);
    g_signal_connect(Salir,"clicked",G_CALLBACK(cerrar),NULL);


    /* Enter the main loop */
    gtk_widget_show(GTK_WIDGET(Ventana1));
    gtk_main ();

    return 0;
}
