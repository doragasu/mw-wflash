% Compute scroll values for non-linear shifts
clear;

% Number of scroll steps (frames)
steps = 20;
% Total number of pixels to shift
px = 320;

% Squared values to shift
squares = (1:steps).^2;
% Total sum of the squared values
total = sum(squares);
% Delta number of pixels to scroll each step
deltas = ceil(px / total * squares);

% Compensate for extra pixels
extra = sum(deltas) - px;
for n = 0:(extra - 1)
	deltas(steps - n) = deltas(steps - n) - 1;
end

% Print comma separated scroll values
printf('%d, ', deltas)
printf('\n')
