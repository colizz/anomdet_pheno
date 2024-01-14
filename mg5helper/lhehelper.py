import os
import re
from logger import _logger

def get_num_evt(path):
    ## Unzip if ends with .gz
    if path.endswith('.gz'):
        os.system('gzip -d ' + path)
        path = path[:-3]
    ## Count event numbers
    nevt = 0
    with open(path) as f:
        for line in f:
            if line.startswith('</event>'):
                nevt += 1
    return nevt

def lhe_split(path, num_split, splitdir='mg5helper_split'):
    dirname = os.path.dirname(path)
    assert not os.path.exists(os.path.join(dirname, splitdir)), "split folder already exists!"
    os.makedirs(os.path.join(dirname, splitdir))
    ## Unzip if ends with .gz
    if path.endswith('.gz'):
        os.system('gzip -d ' + path)
        path = path[:-3]
    
    nevt = get_num_evt(path)
    
    ## Check num_split
    if nevt // 10 + 1 < num_split:
        _logger.warning('Will set num_split to {}'.format(nevt // 10 + 1))
        num_split = nevt // 10 + 1

    ## Get number of event in each splitted file
    nevt_split = [nevt // num_split for _ in range(num_split)]
    nevt_split[-1] = nevt - (nevt // num_split) * (num_split - 1)
    _logger.debug('Split into {} files: [{}]'.format(num_split, ', '.join(map(str, nevt_split))))
    
    # write events
    banner, evt_str = [], []
    flag_banner_end, flag_event_start, flag_event_end = False, False, False
    ifile, ievt = 0, 0
    with open(path) as f:
        for line in f:
            if not flag_banner_end and not line.startswith('<event>'):
                banner.append(line)
            else:
                flag_banner_end = True
            ## Write banner to all splitted files
            if flag_banner_end and not flag_event_start:
                for i in range(num_split):
                    os.makedirs(os.path.join(dirname, splitdir, 'split_%d' % i))
                    with open(os.path.join(dirname, splitdir, 'split_%d' % i, 'events.lhe'), 'w') as fw:
                        fw.write(''.join(banner))
                    
                flag_event_start = True
            # Write events file by file
            if flag_event_start and not flag_event_end:
                evt_str.append(line)
                if line.startswith('</event>'):
                    with open(os.path.join(dirname, splitdir, 'split_%d' % ifile, 'events.lhe'), 'a') as fw:
                        fw.write(''.join(evt_str))
                    evt_str = []
                    ievt += 1
                if ievt == nevt_split[ifile]:
                    _logger.info('Finish writing file {}, nevent: {}'.format(ifile, ievt))
                    ievt = 0
                    ifile += 1
                    if ifile == num_split:
                        flag_event_end = True
                        continue
            if flag_event_end:
                for i in range(num_split):
                    with open(os.path.join(dirname, splitdir, 'split_%d' % i, 'events.lhe'), 'a') as fw:
                        fw.write(line)

    return num_split
