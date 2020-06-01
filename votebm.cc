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
    char title[128], url[128];
    sprintf(title, "t%d", i);
    sprintf(title, "u%d", i);
    write_stories_0(i, i, title, url);
  }

  write_votes_0(17, 25);
  write_votes_0(18, 26);
  write_votes_0(18, 27);

  int xauthor, xcount;
  std::string xtitle, xurl;
  assert(read_output(17, xauthor, xtitle, xurl, xcount) == true);
  assert(xcount == 3);
  assert(read_output(18, xauthor, xtitle, xurl, xcount) == true);
  assert(xcount == 2);
  assert(read_output(19, xauthor, xtitle, xurl, xcount) == false);

  double t0 = now();
  int warmwrites = 0;

#if 0
  // 40 seconds of warm-up.
  // doesn't change results much.
  while(now() - t0 < 40){
    for(int i = 0; i < 100; i++){
      write_votes_0(random() % nstories, random());
      warmwrites++;
    }
  }
#endif

  // now the benchmark: insert lots of votes.
  t0 = now();
  int nwrites = 0;
  while(now() - t0 < 10){
    for(int i = 0; i < 100; i++){
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
    if(read_output(i, xauthor, xtitle, xurl, xcount)){
      total += xcount;
    }
  }
  printf("%d total votes in output view, expecting %d\n",
         total, nwrites+warmwrites+5);

  printf("storieswithvc_data_0.size() %d\n", (int)storieswithvc_data_0.size());
  printf("  load_factor %.2f\n", storieswithvc_data_0.load_factor());
  printf("  max_load_factor %.2f\n", storieswithvc_data_0.max_load_factor());
  printf("  bucket_count %d\n", (int)storieswithvc_data_0.bucket_count());
  printf("  max_bucket_count %d\n", (int)storieswithvc_data_0.max_bucket_count());
  //printf("storieswithvc_data_1.size() %d\n", (int)storieswithvc_data_1.size());
  printf("votecount_data.size() %d\n", (int)votecount_data.size());
  printf("output_data.size() %d\n", (int)output_data.size());
}
