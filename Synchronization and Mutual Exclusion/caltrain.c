#include <pthread.h>
#include "caltrain.h"

/*
initialize the station parameters
set the mutex and condition variables
*/
void
station_init(struct station *station)
{
    station->passengers_waiting=0;
    station->train_free_seats=0;
    station->passengers_standing=0;
    pthread_mutex_init(&station->mutex,NULL);
    pthread_cond_init(&station->train_arrived,NULL);
    pthread_cond_init(&station->passengers_seated,NULL);

}
/*
Invoked when a train arrives in the station and has opened its doors.
Count indicates how many seats are available on the train. The function
does not return until the train is satisfactorily loaded (all passengers are in
their seats, and either the train is full or all waiting passengers have
boarded).
The train must leave the station promptly if no passengers are waiting at
the station or it has no available free seats.
*/
void
station_load_train(struct station *station, int count)
{
    station->train_free_seats=count;
    while(station->train_free_seats>0 && station->passengers_waiting>0){
        pthread_cond_broadcast(&station->train_arrived);
        pthread_cond_wait(&station->passengers_seated,&station->mutex);
    }
    station->train_free_seats=0;

}
/*
Invoked when a passenger robot arrives in a station
Does not return until a train is in the station (i.e., a call to station load train
is in progress) and there are enough free seats on the train for this
passenger to sit down.
Once this function returns, the passenger robot will move the passenger on
board the train and into a seat.
*/
void
station_wait_for_train(struct station *station)
{
    pthread_mutex_lock(&station->mutex);
    station->passengers_waiting++;
    while(station->passengers_standing == station->train_free_seats)
        pthread_cond_wait(&station->train_arrived,&station->mutex);
    station->passengers_standing++;
    station->passengers_waiting--;
}
/*
Once the passenger is seated, it will call this function to let the train know
that it’s on board.
*/
void
station_on_board(struct station *station)
{
    station->train_free_seats--;
    station->passengers_standing--;
    if( station->train_free_seats==0 || station->passengers_standing==0)
      pthread_cond_broadcast(&station->passengers_seated);
    pthread_mutex_unlock(&station->mutex);
}

