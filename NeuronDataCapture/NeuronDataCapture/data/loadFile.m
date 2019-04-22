function [ data ] = loadFile( path, name, matrix, format )
%Load file from path with name specified by matix and format 

sampleFile  = [path name];
[fileID meassage] = fopen(sampleFile, 'r');
if length(meassage) == 0
    data = fread(fileID, matrix, format);
else
    fprintf('Error reading!\n');
end
fclose('all');

end

