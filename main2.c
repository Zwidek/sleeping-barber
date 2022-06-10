#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <time.h>
#include <sys/types.h>
#include <sys/time.h>
#include <string.h>
#include <stdbool.h>

#define MAX_CHAIRS 5  // ilość wolnych miejsc

sem_t barber_ready; // semafor informujący o gotowości fryzjera 
sem_t client_ready; // semafor informujący o gotowści clienta
sem_t seats_ready; //  semafor zliczający miejsca
sem_t printer_ready; // semafor wyświetlający informacje
sem_t leave_ready;  // semafor informujący o wyjściu klienta z lokalu

int taken_seats = 0; // ilość zajętych miejsc
int resigned_count = 0; // ilość nieobsłużonych klientów
bool debug; // flaga, w przypadku true wyświetla kolejkę czekających oraz osób, które nie dostały się do gabinetu
int id = -1; // identyfikator klienta

//https://www.hackerearth.com/practice/data-structures/linked-list/singly-linked-list/tutorial/
struct LinkedList
{
    int data;
    struct LinkedList *next;
};

typedef struct LinkedList *node; //Define node as pointer of data type struct LinkedList

node resigned_list = NULL, waiting_list = NULL;

node createNode()
{
    node temp;                                      // declare a node
    temp = (node)malloc(sizeof(struct LinkedList)); // allocate memory using malloc()
    if (!temp)
    {
        perror("create node malloc error");
    }
    temp->next = NULL; // make next point to NULL
    return temp;       //return the new node
}

node push(node head, int key)
{
    node temp, p;        // declare two nodes temp and p
    temp = createNode(); //createNode will return a new node with data = value and next pointing to NULL.
    temp->data = key;  // add element's value to data part of node
    if (head == NULL)
    {
        head = temp; //when linked list is empty
    }
    else
    {
        p = head; //assign head to p
        while (p->next != NULL)
        {
            p = p->next; //traverse the list until p is the last node.The last node always points to NULL.
        }
        p->next = temp; //Point the previous last node to the new node created.
    }
    return head;
}

node deleteNode(node head, int key)
{
    node temp, p;
    if (key == -1 && head != NULL)
    {
        temp = head->next;
        head = NULL;
        return temp;
    }

    if (head == NULL)
    {
        return head;
    }
    else
    {
        p = head;
        if (p->data == key)
        {
            printf("key deleted: %d\n", p->data);
            head = p->next;

            return head;
        }
        else
        {
            while (p->data != key)
            {
                temp = p;
                p = p->next;
                if (p == NULL)
                {
                    return head;
                }
                if (p->data == key)
                {
                    printf("\nvalue deleted: %d", p->data);
                    temp->next = p->next;
                    return head;
                }
            }
        }
    }
}

// This function prints contents of linked list starting from head
void printList(node head)
{
    node p;
    p = head;
    printf("[");
    while (p != NULL)
    {  
        printf("%d, ", p->data);
        p = p->next;
    }
    printf("] ");
}

// Funkcja wyświetlająca listę klientów oczekujących oraz nieobsłużonych
void summary()
{
    if (debug)
    {
        printf("Resigned list: ");
        printList(resigned_list);
        printf("Waiting list: ");
        printList(waiting_list);
    }
    printf("Res: %d  WRomm: %d/%d  [id: %d]\n\n", resigned_count, taken_seats, MAX_CHAIRS, id);
}


void* barber()
{
    while (1)
    {
        sem_wait(&client_ready);  // oczekiwanie na klienta
        sem_wait(&seats_ready); // oczekiwanie na zmiane miejsc w poczekalni

        taken_seats--; // zmniejszenie liczb miejsc
        
        id = waiting_list->data; // przypisanie klientowi identyfikatora
        waiting_list = deleteNode(waiting_list, -1); // klient zwalnia miejsce w poczekalni

        sem_post(&barber_ready); // fryzjer czeka na klienta
        sem_post(&seats_ready); // klienci zajmuje miejsce

        sleep(3); // czas trwania strzyżenia 
    }
}


void client(void *args)
{

    //klient czeka na sprawdzenie liczby zejętych miejsc 
    sem_wait(&seats_ready);
        
    if (taken_seats < MAX_CHAIRS)
    {
        // klient zajmuje miejsce
        taken_seats++;
        waiting_list = push(waiting_list, *(int *)args);

        //Sprawdza dostępność drukarki
        sem_wait(&printer_ready);
        summary();
        //Umożliwia dostęp do drukarki
        sem_post(&printer_ready);

        // Klient jest gotowy do strzyżenia
        sem_post(&client_ready);

        // Udostępnienie miejsca w poczekalni
        sem_post(&seats_ready);
        // Czeka, aż fryzjer będzie gotowy
        sem_wait(&barber_ready);

        //Sprawdza dostępność drukarki
        sem_wait(&printer_ready);
        summary();
        //Umożliwia dostęp do drukarki
        sem_post(&printer_ready);
    }
    else
    {
        //blokuje możliwość wejścia 
        sem_post(&seats_ready);
        // czeka na wyjście z lokalu klienta
        if (sem_wait(&leave_ready) == -1)
        {
            perror("sem_wait error");
        }
        resigned_count++;
        //dodanie do listy nieobsłużonych klientów
        resigned_list = push(resigned_list, *(int *)args);

         //Sprawdza dostępność drukarki
        sem_wait(&printer_ready);
        summary();
         //Umożliwia dostęp do drukarki
        sem_post(&printer_ready);
        // klient opuszcza lokal
        sem_post(&leave_ready);
    }
    //zamknięcie wątku
    pthread_exit(NULL);
}

void *client_routine()
{
    int rc;
    int i = 0;
    while (true)
    {
        i += 1;
        //tworzy wątek klienta
        pthread_t client_thread;
        rc = pthread_create(&client_thread, NULL, (void *)client, &i);
        sleep(1); // średni czas trwania przybycia nowego klienta
    }
}

int main(int argc, char *argv[])
{
    if (argc > 1)
    {
        char *flag = argv[1];
        if (!strcmp(flag, "debug"))
            debug = true;
    }

    srand(time(NULL));
    
   

    pthread_t create_barber;
    pthread_t create_client;

    //inicjalizacja semaforów
    sem_init(&client_ready, 0, 0);
    sem_init(&barber_ready, 0, 0);
    sem_init(&seats_ready, 0, 1);
    sem_init(&printer_ready, 0, 1);
    sem_init(&leave_ready, 0, 1);

    //tworzenie wątków - klienta i barbera
    int rc;
    rc = pthread_create(&create_barber, NULL, (void *)barber, NULL);
    if (rc)
        printf("Failed to create barber thread");
    rc = pthread_create(&create_client, NULL, (void *)client_routine, NULL);
    if (rc)
        printf("Failed to create ClientMaker thread");

    //synchronizacja
    rc = pthread_join(create_barber, NULL);
    if(rc)
        printf("Failed to join thread");  
    rc = pthread_join(create_client, NULL);
    if(rc)
        printf("Failed to join thread");  

    sem_destroy(&client_ready);
    sem_destroy(&barber_ready);
    sem_destroy(&seats_ready);
    sem_destroy(&printer_ready);
    sem_destroy(&leave_ready);

    return 0;
}