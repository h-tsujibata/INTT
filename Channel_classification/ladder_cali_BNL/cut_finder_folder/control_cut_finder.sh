#!/bin/bash                                                                                                                                  
#echo $1    

folder_direction="/home/5202011/INTT_cal/INTT_cal_test/ladder_cali"
ladder_folder="/ladder_files"
template_version="/template_v1"
cut_finder_folder="/cut_finder_folder" 
while read STRING
do

    echo we are now working on : $STRING
    #cp $folder_direction$cut_finder_folder/cut_file_transport.sh $folder_direction$ladder_folder/$STRING/
    cd $folder_direction$ladder_folder/$STRING/
    
    cp folder*/*.root $folder_direction$cut_finder_folder

    #sh cut_file_transport.sh < total_file.txt

done
cd $folder_direction$cut_finder_folder
rm sum_up_all.root

hadd sum_up_all.root NCU_fphx_raw_module*.root

root -l -b -q cut_finder.c
cd $folder_direction
#root -l -b -q $folder_direction/final_results.c\(\"$folder_direction\"\)



