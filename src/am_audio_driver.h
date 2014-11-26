extern "C" {

extern void AMA_init();
extern int AMA_speakers();
extern void AMA_connect(int from_id, int to_id);

extern int AMA_oscillator_create();
extern void AMA_oscillator_delete(int id);
extern void AMA_oscillator_start(int id, double when);
extern void AMA_oscillator_stop(int id, double when);

}
