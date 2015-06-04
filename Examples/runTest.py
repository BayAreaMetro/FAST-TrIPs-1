import argparse, os, pandas, re, shutil, subprocess, sys

USAGE = r"""

  python runTest.py num_passengers asgn_type iters capacity PSRC|SanFrancisco

  Where asgn_type is one of 'deterministic' or 'stochastic'

  Use capacity=0 to leave it unmodified in trips and configure no capacity constraint.

  Use type='SanFranciso' to convert San Francisco inputs (rename/reorder columns, etc) to input format.
  Use type='PSCRC' to convert PSRC inputs (convert half the demand to inbound, since it's all outbound).

"""

if __name__ == '__main__':
    parser = argparse.ArgumentParser(usage=USAGE)
    parser.add_argument("num_passengers", type=int,
                        help="Number of passengers to assign and simuluate")
    parser.add_argument("asgn_type",      choices=['deterministic','stochastic'])
    parser.add_argument("iters",          type=int)
    parser.add_argument("capacity",       type=int)
    parser.add_argument("type",           choices=['PSRC','SanFrancisco'])

    args = parser.parse_args(sys.argv[1:])
    test_dir = "pax%d_%s_iter%d_%s" % (args.num_passengers, args.asgn_type, args.iters,
                                       "cap%d" % args.capacity if args.capacity > 0 else "nocap")

    if not os.path.exists(test_dir):
        print "Creating test dir [%s]" % test_dir
        os.mkdir(test_dir)

    # copy in the Input
    for input_file in ["ft_input_accessLinks.dat",
                       # "ft_input_shapes.dat",  # turns out this isn't needed
                       "ft_input_transfers.dat"]:
        print "Copying %-30s from Input to %s" % (input_file, test_dir)
        shutil.copy(os.path.join("Input",input_file), os.path.join(test_dir,input_file))

    # Create truncated demand
    input_file  = "ft_input_demand.dat"
    demand_df   = pandas.read_csv(os.path.join("Input", input_file), sep="\t")
    demand_df   = demand_df.loc[demand_df.index < args.num_passengers]
    # PSRC: make every other trip inbound rather than outbound
    if args.type == 'PSRC':
        # can't use mod operator on index so make a temporary column
        demand_df['rownum'] = demand_df.index
        demand_df.loc[demand_df.rownum % 2==0, 'direction'] = 2
        demand_df.drop('rownum', axis=1, inplace=True)
    elif args.type == 'SanFrancisco':
        demand_df.rename(columns={'persid'              :'passengerID',
                                  'mOtaz'               :'OrigTAZ',
                                  'mDtaz'               :'DestTAZ',
                                  'mChosenmode'         :'mode',
                                  'mOdt'                :'timePeriod',
                                  'mSegDir'             :'direction',
                                  'PDT/PAT'             :'PAT'}, inplace=True)
        demand_df = demand_df[['passengerID','OrigTAZ','DestTAZ','mode','timePeriod','direction','PAT']]
    demand_df.to_csv(os.path.join(test_dir, input_file), sep="\t", index=False)
    print "Wrote %8d lines of demand into %s" % (len(demand_df), os.path.join(test_dir, input_file))

    # Routes file
    input_file  = "ft_input_routes.dat"
    routes_df   = pandas.read_csv(os.path.join("Input", input_file), sep="\t")
    if args.type == 'SanFrancisco':
        routes_df.rename(columns={'route_id'            :'routeId',
                                  'route_short_name'    :'routeShortName',
                                  'route_long_name'     :'routeLongName',
                                  'route_type'          :'routeType'}, inplace=True)
    routes_df.to_csv(os.path.join(test_dir, input_file), sep="\t", index=False)
    print "Wrote %8d lines to %s" % (len(routes_df), os.path.join(test_dir, input_file))

    # Stops file
    input_file  = "ft_input_stops.dat"
    stops_df    = pandas.read_csv(os.path.join("Input", input_file), sep="\t")
    if args.type == 'SanFrancisco':
        stops_df.rename(columns={'stop_id'              :'stopId',
                                 'stop_name'            :'stopName',
                                 'stop_lat'             :'Latitude',
                                 'stop_lon'             :'Longitude',
                                 'PassengerCapacity'    :'capacity'}, inplace=True)
        stops_df['stopDescription'] = stops_df['stopName']
        stops_df = stops_df[['stopId','stopName','stopDescription','Latitude','Longitude','capacity']]
    stops_df.to_csv(os.path.join(test_dir, input_file), sep="\t", index=False)
    print "Wrote %8d lines to %s" % (len(stops_df), os.path.join(test_dir, input_file))

    # Stop Times file
    input_file = "ft_input_stopTimes.dat"
    stoptimes_df = pandas.read_csv(os.path.join("Input", input_file), sep="\t")
    if args.type == 'SanFrancisco':
        stoptimes_df.rename(columns={'trip_id'          :'tripId',
                                     'arrival_time'     :'arrivalTime',
                                     'departure_time'   :'departureTime',
                                     'stop_id'          :'stopId',
                                     'stop_sequence'    :'sequence'}, inplace=True)
    stoptimes_df.to_csv(os.path.join(test_dir, input_file), sep="\t", index=False)
    print "Wrote %8d lines to %s" % (len(stoptimes_df), os.path.join(test_dir, input_file))

    # Trips file
    input_file = "ft_input_trips.dat"
    trips_df = pandas.read_csv(os.path.join("Input",input_file), sep="\t")
    if args.type == 'SanFrancisco':
        trips_df.rename(columns={'trip_id'              :'tripId',
                                 'route_id'             :'routeId',
                                 'Capacity'             :'capacity',
                                 'shape_id'             :'shapeId'}, inplace=True)
        trips_df['directionId'] = 0 # Hmmm smarter soln?
    # update capacity
    if args.capacity > 0:
        trips_df['capacity'] = args.capacity
        print "Updated capacity in %s" % os.path.join(test_dir, input_file)
    trips_df.to_csv(os.path.join(test_dir, input_file), sep="\t", index=False)
    print "Wrote %8d lines to %s" % (len(trips_df), os.path.join(test_dir, input_file))

    # TAZ
    input_file = "ft_input_zones.dat"
    zones_df = pandas.read_csv(os.path.join("Input",input_file), sep="\t")
    if args.type == 'SanFrancisco':
        zones_df.rename(columns={'noedId':'ID'}, inplace=True)
    zones_df.to_csv(os.path.join(test_dir,input_file), sep="\t", index=False)
    print "Wrote %8d lines to %s" % (len(zones_df), os.path.join(test_dir, input_file))

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