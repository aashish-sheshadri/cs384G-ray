clear all;
close all;
outerRad = 1;
innerRad = 0.5;
numCirclePoints = 5;
height = 0.2;
outerPoints = pi/2:(2*pi)/numCirclePoints:(5*pi)/2;
innerPoints = outerPoints+(pi/4);
centerUp = [0 0 height];
centerDown = [0 0 -height];

outerX = outerRad*cos(outerPoints);
outerY = outerRad*sin(outerPoints);
outerZ = zeros(1,length(outerX));

outerX(end) = [];
outerY(end) = [];
outerZ(end) = [];

innerX = innerRad*cos(innerPoints);
innerY = innerRad*sin(innerPoints);
innerZ = zeros(1,length(innerX));
innerX(end) = [];
innerY(end) = [];
innerZ(end) = [];

% scatter3(outerX,outerY,outerZ);
% hold on;
% scatter3(innerX,innerY,innerZ);
% scatter3(0,0,height);
% scatter3(0,0,-height);

allPointsIdx = 1:(length(outerX)+length(innerX)+2);
allPointsIdx = allPointsIdx + 1;

stringPoints = 'points = (';
stringFaces = 'faces = (';

fid = fopen('star.txt','w');
fprintf(fid,'%s\n',stringPoints);
fprintf(fid,'%s%5f,%5f,%5f%s,\n','(',centerUp(1),centerUp(2),centerUp(3),')');
fprintf(fid,'%s%5f,%5f,%5f%s,\n','(',centerDown(1),centerDown(2),centerDown(3),')');
for i=1:length(outerX)
    fprintf(fid,'%s%5f,%5f,%5f%s,\n','(',outerX(i),outerY(i),outerZ(i),')');
end
for i=1:length(innerX)
    fprintf(fid,'%s%5f,%5f,%5f%s,\n','(',innerX(i),innerY(i),innerZ(i),')');
end
fprintf(fid,'%s\n',');');
fprintf(fid,'%s\n',stringFaces);

for i=1:length(outerX)
    fprintf(fid,'%s%u,%u,%u%s,\n','(',0,allPointsIdx(i),allPointsIdx(i+length(outerX)),')');
    fprintf(fid,'%s%u,%u,%u%s,\n','(',1,allPointsIdx(i),allPointsIdx(i+length(outerX)),')');
    nextIdx = rem(i,length(outerX));
    if(nextIdx == 0)
        nextIdx = 1;
    end
    fprintf(fid,'%s%u,%u,%u%s,\n','(',0,allPointsIdx(nextIdx),allPointsIdx(i+length(outerX)),')');
    fprintf(fid,'%s%u,%u,%u%s,\n','(',1,allPointsIdx(nextIdx),allPointsIdx(i+length(outerX)),')');
end
fprintf(fid,'%s',');');
fclose(fid);



