#include <stdlib.h>
#include "fsmpp.h"

void
fsmpp::fsm_init(fsmpp_trans_t* tt)
{
  this->tt = tt;
  this->current_state = tt[0].orig_state;
}

/*fsmpp::fsmpp (fsmpp_trans_t* tt)
{
  this->fsm_init (tt);
}*/

fsmpp::fsmpp (){}

void
fsmpp::fsm_fire ()
{
  fsmpp_trans_t* t;
  for (t = this->tt; t->orig_state >= 0; ++t) {
    if ((this->current_state == t->orig_state) && t->in(this)) {
      this->current_state = t->dest_state;
      if (t->out != NULL)
        t->out(this);
      break;
    }
  }
}
