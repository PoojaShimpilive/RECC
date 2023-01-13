 
 To activate BUildGrid RECC_SERVER
 
  450  cd buildgrid/
  451  . env/bin/activate
  452  bgd server start server.yml -vvv
  453  ls
  454  cd buildgrid/
  455  ls
  456  vim server.yml
  457  . env/bin/activate
  458  cd buildgrid/
  459  . env/bin/activate
  460  bgd bot --remote=http://localhost:50051 --instance-name=main host-tools
  461  bgd bot --remote=http://localhost:50051 --instance-name=main host-tools -vvv
  462  bgd bot --remote=http://localhost:50051 --instance-name=main -vvv host-tools

  
  To Start RECC
  
  mkdir build
  486  cd build
  487  sudo cmake .. && sudo make install
  488  env | RECC
  489  export RECC_INSTANCE=main
  490  export RECC_SERVER=localhost:50051
  491  export RECC_VERBOSE=1
  492  cd ../../
  493  cd brotli/
  494  make clean
  495  make CC="recc gcc"
