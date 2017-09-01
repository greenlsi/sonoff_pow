#ifndef FSMPP_H
#define FSMPP_H

class fsmpp;

typedef int (*fsmpp_input_func_t) (fsmpp*);
typedef void (*fsmpp_output_func_t) (fsmpp*);

typedef struct fsmpp_trans_t {
  int orig_state;
  fsmpp_input_func_t in;
  int dest_state;
  fsmpp_output_func_t out;
} fsmpp_trans_t;

class fsmpp {

private:
  int current_state;
  fsmpp_trans_t* tt;

public:
  void* userInfo;

  //fsmpp(fsmpp_trans_t* tt);
  fsmpp();


  void fsm_init(fsmpp_trans_t* tt);
  void fsm_fire();
};

#endif
