
/*
  hp2.c
  Hot potato simulator with hold and reduction
  February 19, 2016
  Justin M. Wozniak
  based on ../airport
*/

#include <ross.h>
#include <rc-stack.h>
#include <inttypes.h>
#include <stdbool.h>

static tw_stime lookahead = 0.00000001;
static int      opt_mem = 1000;

const static tw_stime timeout = 11;
const static tw_stime game_time = 10;
const static tw_stime mean_air_time = 1;
const static tw_stime mean_hold_time = 2;
static uint players = 8;
static uint custom_seed = 0;

const tw_optdef app_opt [] =
{
  TWOPT_GROUP("Hot Potato Model"),
  TWOPT_UINT("players", players,     "Number of players"),
  TWOPT_UINT("seed",    custom_seed, "Random seed"),
  TWOPT_UINT("memory",  opt_mem,     "optimistic memory"),
  TWOPT_END()
};

struct game_state
{
  int catches;
  bool game_on;
  tw_stime last_held;
  tw_stime * stats;
  struct rc_stack * rcs;
};

struct old_catch
{
  tw_stime timestamp;
};

typedef enum
{
  HOLD=1, THROW, GAME_OVER, POTATO_STATUS
} hp_msg_type;

struct hp_message
{
  hp_msg_type type;
  // The following fields are optional:
  tw_stime last_held;
  tw_lpid sender;
};

static tw_peid
mapping(tw_lpid gid)
{
  return (tw_peid) gid / g_tw_nlp;
}

static void throw_potato(struct game_state * s, tw_lp * lp);

static inline struct old_catch*
old_catch_create(tw_stime timestamp)
{
  struct old_catch * result = malloc(sizeof(struct old_catch*));
  result->timestamp = timestamp;
  return result;
}

static inline void
old_catch_push(struct game_state * s, tw_lp * lp)
{
  struct old_catch* catch = old_catch_create(s->last_held);
  rc_stack_push(lp, catch, free, s->rcs);
  s->last_held = tw_now(lp);
  rc_stack_gc(lp, s->rcs);
}

static inline void
old_catch_pop(struct game_state * s)
{
  struct old_catch * catch = rc_stack_pop(s->rcs);
  s->last_held = catch->timestamp;
  free(catch);
}

static void
init(struct game_state * s, tw_lp * lp)
{
  s->catches = 0;
  s->game_on = true;
  s->last_held = -1;
  s->stats = NULL;

  rc_stack_create(&s->rcs);

  // Send self an end of game notice
  printf("Me: %"PRIu64"\n", lp->gid);
  tw_event *e = tw_event_new(lp->gid, game_time, lp);
  struct hp_message *m = tw_event_data(e);
  m->type = GAME_OVER;
  tw_event_send(e);

  if (lp->gid == 0)
  {
    // Start game
    throw_potato(s, lp);
    // Allocate space for post-game statistics
    s->stats = calloc(players, sizeof(tw_stime));
  }
}

static void
catch_potato(struct game_state * s, tw_lp * lp)
{
  printf("%0.2f CATCH %"PRIu64"\n", tw_now(lp), lp->gid);
  if (! s->game_on) return;
  int c = tw_rand_integer(lp->rng, 0, 30);
  if (c == 0)
  {
    printf("Player %"PRIu64" dropped the potato!\n", lp->gid);
    return; // without throw
  }
  old_catch_push(s, lp);
  s->catches++;
  tw_stime hold_time = tw_rand_exponential(lp->rng, mean_hold_time);
  printf("HOLD  %0.1f\n", hold_time);

  tw_event *e = tw_event_new(lp->gid, hold_time, lp);
  struct hp_message *m = tw_event_data(e);
  m->type = HOLD;
  tw_event_send(e);
}

static void
catch_potato_rc(struct game_state * s, tw_lp * lp)
{
  printf("%0.2f RC CATCH %"PRIu64"\n", tw_now(lp), lp->gid);
  if (! s->game_on) return;
  s->catches--;
  old_catch_pop(s);
  tw_rand_reverse_unif(lp->rng);
  tw_rand_reverse_unif(lp->rng);
}

static tw_lpid
select_catcher(tw_lp * thrower)
{
  tw_lpid catcher =  thrower->gid;
  do      catcher =  tw_rand_integer(thrower->rng, 0, players-1);
  while  (catcher == thrower->gid);
  printf("%0.2f Throw from %" PRIu64 " to %" PRIu64"\n",
          tw_now(thrower),    thrower->gid,  catcher);
  return catcher;
}

static void
throw_potato(struct game_state * s, tw_lp * lp)
{
  if (! s->game_on) return;
  tw_lpid catcher = select_catcher(lp);
  tw_stime air_time = tw_rand_exponential(lp->rng, mean_air_time);
  tw_event *e = tw_event_new(catcher, air_time, lp);
  struct hp_message *m = tw_event_data(e);
  m->type = THROW;
  tw_event_send(e);
}

static void
throw_potato_rc(struct game_state * s, tw_lp * lp)
{
  printf("%0.2f RC THROW %"PRIu64"\n", tw_now(lp), lp->gid);
  if (! s->game_on) return;
  tw_rand_reverse_unif(lp->rng);
  tw_rand_reverse_unif(lp->rng);
}

static void
send_status(struct game_state * s, tw_lp * lp)
{
  // printf("GAME_OVER %"PRIu64"\n", lp->gid);
  s->game_on = false;
  tw_lpid master = 0;
  tw_event *e = tw_event_new(master, 0.1, lp);
  struct hp_message *m = tw_event_data(e);
  m->type      = POTATO_STATUS;
  m->sender    = lp->gid;
  m->last_held = s->last_held;
  tw_event_send(e);
}

static void
send_status_rc(struct game_state * s, tw_lp * lp)
{
  printf("RC GAME_OVER %"PRIu64"\n", lp->gid);
  s->game_on = true;
}

static void
collect_status(struct game_state * s, tw_lp * lp, struct hp_message * msg)
{
  int slot = msg->sender;
  // printf("STAT  %i\n", slot);
  s->stats[slot] = msg->last_held;
  tw_stime max = 0;
  int last_holder = -1;
  for (int i = 0; i < players; i++)
  {
    if (s->stats[i] == 0) return; // Waiting for stats
    if (s->stats[i] > max)
    {
      max = s->stats[i];
      last_holder = i;
    }
  }
  // printf("The loser is: %i\n", last_holder);
  tw_output(lp, "The loser is: %i\n", last_holder);
}

static void
collect_status_rc(struct game_state * s, tw_lp * lp, struct hp_message * msg)
{
  int slot = msg->sender;
  printf("RC STAT  %i\n", slot);
  s->stats[slot] = 0;
}

static void
event_handler(struct game_state * s, tw_bf * bf, struct hp_message * msg, tw_lp * lp)
{
  switch (msg->type)
  {
    case THROW:
    {
      catch_potato(s, lp);
      break;
    }
    case HOLD:
    {
      throw_potato(s, lp);
      break;
    }
    case GAME_OVER:
    {
      send_status(s, lp);
      break;
    }
    case POTATO_STATUS:
    {
      collect_status(s, lp, msg);
      break;
    }
    default:
    {
      printf("Unknown message type!\n");
      exit(EXIT_FAILURE);
    }
  }
}

static void
rc_event_handler(struct game_state * s, tw_bf * bf, struct hp_message * msg, tw_lp * lp)
{
  switch (msg->type)
  {
    case THROW:
    {
      catch_potato_rc(s, lp);
      break;
    }
    case HOLD:
    {
      throw_potato_rc(s, lp);
      break;
    }
    case GAME_OVER:
    {
      send_status_rc(s, lp);
      break;
    }
    case POTATO_STATUS:
    {
      collect_status_rc(s, lp, msg);
      break;
    }
    default:
    {
      printf("Unknown message type!\n");
      exit(EXIT_FAILURE);
    }
  }
}

static void
final(struct game_state * s, tw_lp * lp)
{
  printf("Player %"PRIu64" had %i catches.\n",
                   lp->gid,    s->catches);
}

tw_lptype player_lps[] =
{
  {
    (init_f) init,
    (pre_run_f) NULL,
    (event_f) event_handler,
    (revent_f) rc_event_handler,
    (final_f) final,
    (map_f) mapping,
    sizeof(struct game_state),
  },
  {0},
};

int
main(int argc, char **argv)
{
  tw_opt_add(app_opt);

  tw_init(&argc, &argv);

  g_tw_ts_end = timeout;
  g_tw_events_per_pe = 1000;
  g_tw_lookahead = lookahead;

  int nlp_per_pe = players / tw_nnodes();
  tw_define_lps(nlp_per_pe, sizeof(struct hp_message));

  for(int i = 0; i < g_tw_nlp; i++)
  {
    tw_lp_settype(i, &player_lps[0]);
    tw_rand_initial_seed(&g_tw_lp[i]->rng[0],
                         custom_seed + g_tw_lp[i]->gid);
  }

  tw_run();
  tw_end();

  return EXIT_SUCCESS;
}
