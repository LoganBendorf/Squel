skip -gfi /usr/include/c++/*
skip -gfi /usr/include/bits/*
skip -gfi /usr/lib/gcc/*
skip function std::*
skip function __gnu_cxx::*# Skip standard library functions
skip -gfi /usr/include/c++/*
skip -gfi /usr/include/c++/v1/*
skip -gfi /usr/lib/gcc/*
skip function std::*
skip function __gnu_cxx::*
skip -rfu ^std::
skip -rfu ^__.*