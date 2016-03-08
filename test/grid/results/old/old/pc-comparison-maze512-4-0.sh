./run_scen.py -scen myscen/mazes/maze512-4-0.map.scen -prog ./select_algo.py -algo "wPWSA*","wPWSA*PC" -proc 1,2,4,8,16,24,32 -w 1.0,1.5,2.0 -output results/pc-comparison-maze512-4-0.txt -runs 2 -attempts 3
./reformat_compare.py -algo1 "wPWSA*PC" -algo2 "wPWSA*" -input results/pc-comparison-maze512-4-0.txt -output results_compare.txt
./reformat_plot2.py -algo1 "wPWSA*PC" -algo2 "wPWSA*" -input results/pc-comparison-maze512-4-0.txt -output results_plot2.txt
pplot scatter -x proc -y speedup -series w -input results_compare.txt -output plots-speedup-maze.pdf
pplot scatter -x proc -y deviation -series algo -chart w -input results_plot2.txt -output plots-deviation-maze.pdf
