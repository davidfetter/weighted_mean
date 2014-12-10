Weighted Stats extension for PostgreSQL
======================================

This extension provides weighted statistical aggregate functions for
PostgreSQL.  It is based on the work of Ronan Dunklau and Peter
Eisentraut in the weighted_mean package.

The weighted stats are defined as two-column aggregation functions.
The first column contains the measure to be averaged, the second
contains the weight of each measure.

This extension is implemented as a C library.


Installation
============

You can either install it with the pgxn client:

  pgxn install weighted_stats

Or from source:

  git clone git://github.com/davidfetter/weighted_stats.git
  make && sudo make install

Then, create the extension in the specific database:

  CREATE extension weighted_stats;


Usage
=====

  select weighted_stats(unitprice, quantity) from sales;

This is equivalent to:

  select 
  case 
    when sum(quantity) = 0 then 0
    else sum(unitprice * quantity) / sum(quantity) 
  end
  from sales;
