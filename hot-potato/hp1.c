
/*
  hp.c
  Hot potato simulator
  February 17, 2016
  Justin M. Wozniak
  based on ../airport
*/

#include <ross.h>
#include <inttypes.h>

static tw_stime lookahead = 0.00000001;
static int      opt_mem = 1000;

const static tw_stime mean_air_time = 1;
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
};

static tw_peid
mapping(tw_lpid gid)
{
  return (tw_peid) gid / g_tw_nlp;
}

static tw_lpid
select_catcher(tw_lp * thrower)
{
  tw_lpid catcher =  thrower->gid;
  do      catcher =  tw_rand_integer(thrower->rng, 0, players-1);
  while  (catcher == thrower->gid);
  printf("Throw from %" PRIu64 " to %" PRIu64"\n",
                        thrower->gid,  catcher);
  return catcher;
}

static void
throw_potato(tw_lp * lp)
{
  tw_lpid catcher = select_catcher(lp);
  tw_stime air_time = tw_rand_exponential(lp->rng, mean_air_time);
  tw_event *e = tw_event_new(catcher, air_time, lp);
  tw_event_send(e);
}

static void
init(struct game_state * s, tw_lp * lp)
{
  s->catches = 0;
  if (lp->gid == 0) throw_potato(lp);
}

static void
event_handler(struct game_state * s, tw_bf * bf, void * msg, tw_lp * lp)
{
  int c = tw_rand_integer(lp->rng, 0, 10);
  if (c == 0)
  {
    printf("Player %"PRIu64" dropped the potato!\n", lp->gid);
    return; // without throw
  }

  s->catches++;
  throw_potato(lp);
}

static void
rc_event_handler(struct game_state * s, tw_bf * bf, void * msg, tw_lp * lp)
{
  s->catches--;
  tw_rand_reverse_unif(lp->rng);
  tw_rand_reverse_unif(lp->rng);
  tw_rand_reverse_unif(lp->rng);
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

  g_tw_ts_end = 10;
  g_tw_events_per_pe = 1000;
  g_tw_lookahead = lookahead;

  int nlp_per_pe = players / tw_nnodes();
  tw_define_lps(nlp_per_pe, 1);

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
