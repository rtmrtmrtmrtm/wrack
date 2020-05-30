#include <sys/time.h>
#include <assert.h>
#include "votebm-schema.cc"

double
now()
{
  struct timeval tv;
  gettimeofday(&tv, 0);
  return tv.tv_sec + tv.tv_usec / 1000000.0;
}

int
main(int argc, char *argv[])
{
  int nstories = 500000; // Noria paper, Section 8.2

  // sanity check.
  write_votes_0(17, 23);
  write_votes_0(17, 24);

  for(int i = 0; i < nstories; i++){
    write_stories_0(i, i);
  }

  write_votes_0(17, 25);
  write_votes_0(18, 26);
  write_votes_0(18, 27);

  int xauthor, xcount;
  assert(read_output(17, xauthor, xcount) == true);
  assert(xcount == 3);
  assert(read_output(18, xauthor, xcount) == true);
  assert(xcount == 2);
  assert(read_output(19, xauthor, xcount) == false);

  // now the benchmark: insert lots of votes.
  double t0 = now();
  int nwrites = 0;
  while(now() - t0 < 10){
    for(int i = 0; i < 10; i++){
      write_votes_0(random() % nstories, random());
      nwrites++;
    }
  }
  double t1 = now();
  printf("%d writes, %.1f seconds, %.0f writes/second\n",
         nwrites, t1 - t0, nwrites / (t1 - t0));

  // sum up the votes, as another sanity check.
  int total = 0;
  for(int i = 0; i < nstories; i++){
    if(read_output(i, xauthor, xcount)){
      total += xcount;
    }
  }
  printf("%d total votes in output view\n", total);

  printf("storieswithvc_data_0.size() %d\n", (int)storieswithvc_data_0.size());
  //printf("storieswithvc_data_1.size() %d\n", (int)storieswithvc_data_1.size());
  printf("votecount_data.size() %d\n", (int)votecount_data.size());
  printf("output_data.size() %d\n", (int)output_data.size());
}
