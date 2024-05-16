# didYouMean
I needed something like Google's Did You Mean feature. This is that.

It uses the Levenshtein distance algorithm and is written in pure C. 

The idea was for a Visual FoxPro application to produce a large ~20 MB .DBF file and have this command line application scan it quickly.

This program depends on a specific DBASE/Fox Pro file format, read the source code to see what it inputs and what it outputs: it's a fixed column width format with one record per row, a header, and a footer.

More info: https://en.wikipedia.org/wiki/Levenshtein_distance
