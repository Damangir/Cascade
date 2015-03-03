# Scenario 1: You have FLAIR or T2 and Freesurfer results

cascade.pipeline.modelfree.freesurfer.sh

# Scenario 2: You have FLAIR or T2 and T1

## You have brain tissue segmentation
```sh
$ cascade.pipeline.modelfree.bts.sh
```
## You don't have brain mask
Create the brain tissue segmentation using the following command and then follow the above instruction

```sh
$ cascade.extra.bts.sh
```