/*
 * Measures worst case latencies visible to the application for one thread that
 * does not mutate the heap and N threads that do.
 */
public class GcLatencyTest {
  public static void main(String[] args) throws Exception {
    if (args.length != 2)
      usage();

    int mutatorCount = Integer.parseInt(args[0]);

    final int cycles = Integer.parseInt(args[1]);

    System.out.println("Running " + mutatorCount + " mutators and 1 non-mutator for " + cycles + " million cycles...");

    System.out.println("Thread       min/avg/max (ns)");

    Thread nonMutator = new Thread(new Runnable() {
      long minDuration = Long.MAX_VALUE;
      long totalDuration;
      long maxDuration;
      int cycle;
      int hash;

      public void run() {

        for (cycle = 0; cycle < cycles * 1000000; cycle++) {
          long start = System.nanoTime();

          long end = System.nanoTime();

          long duration = end - start;

          totalDuration += duration;
          minDuration = Math.min(duration, minDuration);
          maxDuration = Math.max(duration, maxDuration);
        }

        long avgDuration = totalDuration / (cycles * 1000000);

        System.out.println(String.format("No mutation  %d/%d/%d",
            (long) (minDuration),
            (long) (avgDuration),
            (long) (maxDuration)));
      }
    });

    Thread[] mutators = new Thread[mutatorCount];

    for (int i = 0; i < mutators.length; i++) {
      final int index = i;

      mutators[index] = new Thread(new Runnable() {
        long minDuration = Long.MAX_VALUE;
        long totalDuration;
        long maxDuration;
        int cycle;
        int hash;

        public void run() {

          for (cycle = 0; cycle < cycles * 1000000; cycle++) {
            long start = System.nanoTime();

            /*
             * Calculate a hash code for it to avoid Hotspot optimizing out the
             * allocation.
             */
            Object object = new Object();
            hash |= object.hashCode();

            long end = System.nanoTime();

            long duration = end - start;

            totalDuration += duration;
            minDuration = Math.min(duration, minDuration);
            maxDuration = Math.max(duration, maxDuration);
          }

          long avgDuration = totalDuration / (cycles * 1000000);

          System.out.println(String.format("Mutator %-4d %d/%d/%d",
              index,
              (long) (minDuration),
              (long) (avgDuration),
              (long) (maxDuration)));
        }
      });
    }

    for (int i = 0; i < mutators.length; i++)
      mutators[i].start();

    nonMutator.start();

    for (int i = 0; i < mutators.length; i++)
      mutators[i].join();

    nonMutator.join();
  }

  private static void usage() {
    System.err.println("usage: GcLatencyTest <mutator-count> <million-cycles>");
    System.exit(1);
  }
}
