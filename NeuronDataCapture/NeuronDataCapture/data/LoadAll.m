clear;
close all;
fileList = dir('LX_20190420_18*.txt');

NUM_CH = 32;

for i=1:size(fileList,1)
    channelName = fileList(i).name;
    if (channelName(20) == 'D')
        chNumber(i) = str2double(channelName(21:22));
        channel = load(channelName);
        channels(:,i) = channel(1:end);
    end
    if (channelName(20) == 'H')
        header = load(channelName);
    end
    if (channelName(20) == 'C')
        result = load(channelName);
        costs = result(:,1); % First field cost
        activeCh = result(:,2); % Second field active channel
        maximum = result(:,3:NUM_CH+2); % 3-34 Peak value 
        average = result(:,NUM_CH+3:2*NUM_CH+2); % 35-66 Average value
    end
end

%start = 54473;
start = 1;
length = 30000-1;
figure,
surf(channels(start:start+length,:));
xlabel('channels');
ylabel('samples');
zlabel('amplitude');

figure
plot(header(start:start+length,3));
xlabel('samples');
% 0 = stopped
% 1 = measure average
% 2 = search maximum peak
title('Sequence number');
ylabel('sequence');
xlabel('sample');

ch1 = channels(start:start+length, 1);
ch2 = channels(start:start+length, 2);
ch11 = channels(start:start+length, 11);
ch12 = channels(start:start+length, 12);
ch21 = channels(start:start+length, 21);
ch22 = channels(start:start+length, 22);
ch31 = channels(start:start+length, 31);
ch32 = channels(start:start+length, 32);
figure,
plot(ch1);
hold on;
plot(ch2);
plot(ch11);
plot(ch12);
plot(ch21);
plot(ch22);
plot(ch31);
plot(ch32);
title('Channels (1,2,11,12,21,22,31,32)');
ylabel('amplitude');
xlabel('sample');

%idxZero = header(start:start+length, 3) < 2;
diff = ch32 - ch31;
%diff(idxZero) = 0;
figure,
plot(diff);
title('Difference between channel 31 and 32');
ylabel('amplitude');
xlabel('sample');
max(diff)

%figure,
%plot(costs)
%title('Cost');
%ylabel('amplitude');
%xlabel('sample');
%mean(costs)

