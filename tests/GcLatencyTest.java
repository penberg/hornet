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

    System.out.println("Thread       Latency (usec)");

    Thread nonMutator = new Thread(new Runnable() {
      long maxDuration;
      int cycle;
      int hash;

      public void run() {

        for (cycle = 0; cycle < cycles * 1000000; cycle++) {
          long start = System.nanoTime();

          long end = System.nanoTime();

          long duration = end - start;

          maxDuration = Math.max(duration, maxDuration);
        }

        System.out.println(String.format("No mutation  %d", (long) (maxDuration / 1000.0)));
      }
    });

    Thread[] mutators = new Thread[mutatorCount];

    for (int i = 0; i < mutators.length; i++) {
      final int index = i;

      mutators[index] = new Thread(new Runnable() {
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

            maxDuration = Math.max(duration, maxDuration);
          }

          System.out.println(String.format("Mutator %-4d %d", index, (long) (maxDuration / 1000.0)));
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
