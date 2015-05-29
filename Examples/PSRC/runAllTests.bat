
:: Run a set of FAST-TrIPs tests
::
::   python runTest.py num_passengers asgn_type iters capacity
::

:pax200_deterministic_iter1_nocap
python ..\runTest.py 200 deterministic 1 0

:pax200_deterministic_iter1_cap1
python ..\runTest.py 200 deterministic 1 1

:pax200_deterministic_iter2_cap1
python ..\runTest.py 200 deterministic 2 1

:: stochastic
:pax200_stochastic_iter1_nocap
python ..\runTest.py 200 stochastic 1 0

:pax200_stochastic_iter1_cap1
python ..\runTest.py 200 stochastic 1 1

:pax200_stochastic_iter2_cap1
python ..\runTest.py 200 stochastic 2 1