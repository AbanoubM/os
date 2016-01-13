#include <pthread.h>

struct station {
    int passengers_waiting;
    int train_free_seats;
    int passengers_standing;
	pthread_mutex_t mutex;
    pthread_cond_t train_arrived;
    pthread_cond_t passengers_seated;
};

void station_init(struct station *station);

void station_load_train(struct station *station, int count);

void station_wait_for_train(struct station *station);

void station_on_board(struct station *station);
