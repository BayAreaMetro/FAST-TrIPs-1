
:: Run a set of FAST-TrIPs tests
::
::   python runTest.py num_passengers asgn_type iters capacity
::

:pax100_deterministic_iter1_nocap
python runTest.py 100 deterministic 1 0

:pax100_deterministic_iter1_cap1
python runTest.py 100 deterministic 1 1

:pax100_deterministic_iter2_cap1
python runTest.py 100 deterministic 2 1

:: stochastic
:pax100_stochastic_iter1_nocap
python runTest.py 100 stochastic 1 0
