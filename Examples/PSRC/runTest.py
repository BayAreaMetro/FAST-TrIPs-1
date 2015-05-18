import argparse, os, re, shutil, subprocess, sys

USAGE = r"""

  python runTest.py num_passengers asgn_type iters capacity

  Where asgn_type is one of 'deterministic' or 'stochastic'

  Use capacity=0 to leave it unmodified in trips and configure no capacity constraint.

"""

if __name__ == '__main__':
    parser = argparse.ArgumentParser(usage=USAGE)
    parser.add_argument("num_passengers", type=int,
                        help="Number of passengers to assign and simuluate")
    parser.add_argument("asgn_type", choices=['deterministic','stochastic'])
    parser.add_argument("iters", type=int)
    parser.add_argument("capacity", type=int)

    args = parser.parse_args(sys.argv[1:])
    test_dir = "pax%d_%s_iter%d_%s" % (args.num_passengers, args.asgn_type, args.iters,
                                          "cap%d" % args.capacity if args.capacity > 0 else "nocap")

    if not os.path.exists(test_dir):
        print "Creating test dir [%s]" % test_dir
        os.mkdir(test_dir)

    # copy in the Input
    for input_file in ["ft_input_accessLinks.dat",
                       "ft_input_routes.dat",
                       "ft_input_shapes.dat",
                       "ft_input_stops.dat",
                       "ft_input_stopTimes.dat",
                       "ft_input_transfers.dat",
                       "ft_input_zones.dat"]:
        print "Copying %-30s from Input to %s" % (input_file, test_dir)
        shutil.copy(os.path.join("Input",input_file), os.path.join(test_dir,input_file))

    # Create truncated demand
    demand_in   = open(os.path.join("Input" ,"ft_input_demand.dat"))
    demand_out  = open(os.path.join(test_dir, "ft_input_demand.dat"), 'w')
    demand_pat  = re.compile(r"(.*\t.*\t.*\t.*\t.*\t)(1|2)(\t.*)")
    for pax in range(args.num_passengers+1):
        line = demand_in.readline()
        # make every other trip inbound rather than outbound
        if pax % 2 == 1: line = demand_pat.sub(r"\g<1>2\g<3>",line)
        demand_out.write(line)
    demand_in.close()
    demand_out.close()
    print "Wrote %d lines of demand into %s" % (args.num_passengers, os.path.join(test_dir, "ft_input_demand.dat"))

    # Create updated capacity
    input_file = "ft_input_trips.dat"
    if args.capacity <= 0:
        # no update -- just copy
        print "Copying %-30s from Input to %s" % (input_file, test_dir)
        shutil.copy(os.path.join("Input",input_file), os.path.join(test_dir,input_file))
    else:
        trips_in    = open(os.path.join("Input",  input_file))
        trips_out   = open(os.path.join(test_dir, input_file), 'w')
        trips_pat   = re.compile(r"(.*\t.*\t.*\t.*\t)\d+(\t.*\t.*)")
        for line in trips_in:
            line = trips_pat.sub(r"\g<1>%d\g<2>" % args.capacity, line)
            trips_out.write(line)
        trips_in.close()
        trips_out.close()
        print "Updated capacity in %s" % os.path.join(test_dir, input_file)

    # parameters
    # 1   :Number of Iterations
    # 1   :No Assignment(0), Deterministic Assignment(1), Stochastic Assignment(2)
    input_file = "ft_input_parameters.dat"
    parameters_in   = open(os.path.join("Input", input_file))
    parameters_out  = open(os.path.join(test_dir,input_file), 'w')
    line_num        = 1
    for line in parameters_in:
        if line_num == 1:
            parameters_out.write("%d\t:Number of Iterations\n" % args.iters)
        elif line_num == 2:
            parameters_out.write("%d\t:No Assignment(0), Deterministic Assignment(1), Stochastic Assignment(2)\n" % \
                                 (1 if args.asgn_type == 'deterministic' else 2))
        else:
            parameters_out.write(line)
        line_num += 1
    parameters_in.close()
    parameters_out.close()
    print "Updated parameters in %s" % os.path.join(test_dir, input_file)

    # route choices
    # 0       :Capacity Constraint
    input_file = "ft_input_routeChoice.dat"
    parameters_in   = open(os.path.join("Input", input_file))
    parameters_out  = open(os.path.join(test_dir,input_file), 'w')
    line_num        = 1
    for line in parameters_in:
        if line_num == 12:
            parameters_out.write("%d\t\t:Capacity Constraint\n" % (0 if args.capacity <= 0 else 1))
        else:
            parameters_out.write(line)
        line_num += 1
    parameters_in.close()
    parameters_out.close()
    print "Updated parameters in %s" % os.path.join(test_dir, input_file)

    # Run the test, putting output into RunLog.txt
    runlog = open(os.path.join(test_dir,"RunLog.txt"), 'w')
    cmd    = os.path.join("..","..","FT.exe")
    proc   = subprocess.Popen(cmd, cwd=test_dir,
                              stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    for line in proc.stdout:
        line = line.strip("\r\n")
        runlog.write(line+"\n")
        print "stdout: " + line

    for line in proc.stderr:
        line = line.strip('\r\n')
        runlog.write(line+"\n")
        print "stderr: " + line
    retcode  = proc.wait()
    runlog.close()
    print "Received %d from [%s]" % (retcode, cmd)