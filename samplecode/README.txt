estr_plot.c is a simple example program for use with the userspace variant of
estrella. It performs a scan on the first USB2EPP device it can find and
displays the result using Gnuplot. You should well be able to derive your own
application code from this example.

In order to build estr_plot.c you need to have estrella (obviously), libdll (see
http://github.com/bjoernr/libdll) and libgpif (see
http://github.com/bjoernr/libgpif) installed as well as Gnuplot
(http://www.gnuplot.info/).

Then simply compile the program like this:

gcc -o estrplot 
    -I/opt/libgpif/include/gpif 
    -I/opt/libdll/include/dll 
    -I/opt/estrella/include 
    -L/opt/libdll/lib 
    -L/opt/libgpif/lib 
    -L/opt/estrella/lib
    -lgpif 
    -ldll 
    -lestrella
    estr_plot.c

Adapt the include and library paths to your specific installation. This should
only be necessary if you installed the dependencies to a non-standard location.
Otherwise the respective options can be omitted. 

Run the resulting estrplot binary and hope for the best;)

You probably want to slightly edit the source and modify the defines for rate,
resolution and calibration factors (C1, C2, C3) at the very top of the file to
suit your needs.
