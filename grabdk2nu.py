import os

for i in range(0,120):
    #os.system("wget https://portal.nersc.gov/project/dune/data/misc/NuMI_dk2nu/newtarget-200kA_20220409/g4numiv6_minervame_me000z-200i_"+str(i)+"_0001.dk2nu --no-check-certificate")

    os.system("wget https://portal.nersc.gov/project/dune/data/misc/NuMI_dk2nu/newtarget200kA_20210920/g4numiv6_minervame_me000z200i_"+str(i)+"_0001.dk2nu --no-check-certificate")
