clear all;
close all;
numStars = 28;

for i = 1:numStars
    writeStar(['star' num2str(i,'%.2d')]);
end