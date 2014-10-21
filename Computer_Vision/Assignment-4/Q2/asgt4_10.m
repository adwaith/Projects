clc
targets=[1 0];
outputs=[0.7 0.7];
[tpr,fpr,thresholds] = roc(targets,outputs) 
plotroc(targets,outputs)