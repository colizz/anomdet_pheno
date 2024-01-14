import subprocess
import multiprocessing
import os, shutil
import re
from logger import _logger
from lhehelper import get_num_evt, lhe_split
import argparse
parser = argparse.ArgumentParser('MG5 helper')
parser.add_argument('--cfg', default='cards/config.yml', help='YAML config to get global vars.')
parser.add_argument('-m', '--run-madgraph', action='store_true', help='Also launch the MadGraph step')
parser.add_argument('--mdir', default=None, help='Directory of an existing MadGraph process (if running MadGraph step)')
parser.add_argument('--nevent', default=None, help='Number of events to generate (if running MadGraph step)')
parser.add_argument('-i', '--inputfile', help='Path to the input LHE file')
parser.add_argument('-n', '--num-split', type=int, default=-1, help='Number of splitted runs')
parser.add_argument('--pcard', default=None, help='Path to the Pythia card produced from the MG test run.')
parser.add_argument('--dcard', default=None, help='Path to the Delphes card.')
parser.add_argument('-t', '--output-tag', default='tag', help='Output tag of the combined Delphes ROOT file.')
parser.add_argument('--testrun', action='store_true', help='Run one file for test.')
parser.add_argument('-f', '--force', action='store_true', help='Force to remove the split folder and re-run.')
args = parser.parse_args()

def runcmd(cmd, logpath=None):
    """Run a shell command"""
    if logpath is None:
        p = subprocess.Popen(cmd, shell=True, universal_newlines=True)
        out, _ = p.communicate()
    else:
        with open(logpath, 'w') as fout:
            p = subprocess.Popen(cmd, shell=True, universal_newlines=True, stderr=fout, stdout=fout)
            out, _ = p.communicate()
    return (out, p.returncode)

def run_simulation(splitdir, args, config):
    ## Copy pythia8 file
    if not os.path.exists(args.pcard):
        _logger.error('Pythia8 card does not exists')
    with open(args.pcard) as f, open(os.path.join(splitdir, 'PY8Card.dat'), 'w') as fw:
        for line in f:
            if line.startswith('Main:numberOfEvents'):
                fw.write('Main:numberOfEvents      = {}\n'.format(get_num_evt(os.path.join(splitdir, 'events.lhe'))))
            elif line.startswith('HEPMCoutput:file'):
                fw.write('HEPMCoutput:file         = events.hepmc\n')
            elif line.startswith('Beams:LHEF'):
                fw.write('Beams:LHEF               = events.lhe\n')
            elif line.startswith('HEPMCoutput:scaling'):
                scale = float(re.findall('.*=\s*(\S+)$', line)[0])
                fw.write('HEPMCoutput:scaling = %E\n' % (scale/args.num_split))
            else:
                fw.write(line)
    ## Run Pythia8
    _logger.info('{}: Runing Pythia8'.format(os.path.basename(splitdir)))
    with open(os.path.join(splitdir, 'run_pythia8.sh'), 'w') as fw:
        fw.write('rm -f events.hepmc; LD_LIBRARY_PATH={path}/HEPTools/lib:$LD_LIBRARY_PATH {path}/HEPTools/MG5aMC_PY8_interface/MG5aMC_PY8_interface PY8Card.dat'.format(path=config['MG5_path']))
    runcmd(
        cmd='cd {}; bash run_pythia8.sh'.format(splitdir),
        logpath=os.path.join(splitdir, 'pythia8.log') if not args.testrun else None
    )
    ## Run Delphes a.root events.hepmc
    if args.dcard is None:
        _logger.info('Skip Delphes step.')
    elif not os.path.exists(args.dcard):
        _logger.error('Delphes card does not exists')
    else:
        _logger.info('{}: Runing Delphes'.format(os.path.basename(splitdir)))
        with open(os.path.join(splitdir, 'run_delphes.sh'), 'w') as fw:
            fw.write('rm -f events_delphes.root; {path}/DelphesHepMC2 {dcard} events_delphes.root events.hepmc'.format(path=config['Delphes_path'], dcard=args.dcard))
        runcmd(
            cmd='cd {}; bash run_delphes.sh'.format(splitdir),
            logpath=os.path.join(splitdir, 'delphes.log') if not args.testrun else None
        )
    _logger.info('Job finished in {}'.format(splitdir))

def combine_root(basedir, splitdir, args):
    _logger.info('Combining root file.')
    with open(os.path.join(basedir, 'run_combine_root.sh'), 'w') as fw:
        fw.write('rm -f delphes_{tag}.root; hadd delphes_{tag}.root '.format(tag=args.output_tag) + ' '.join([os.path.join(basedir, splitdir, 'split_%d' % i, 'events_delphes.root') for i in range(args.num_split)]))
    runcmd(
        cmd='cd {}; bash run_combine_root.sh'.format(basedir),
        logpath=os.path.join(basedir, 'combine_root.log')
    )
    _logger.info('All root combined. File available in {basedir}/delphes_{tag}.root'.format(basedir=basedir, tag=args.output_tag))

if __name__ == '__main__':
    import yaml
    dir_path = os.path.dirname(os.path.realpath(__file__))
    with open(os.path.join(dir_path, args.cfg)) as f:
        config = yaml.safe_load(f)

    ## Auto set the num_split to the number of cpu
    if args.num_split == -1:
        args.num_split = multiprocessing.cpu_count()

    ## Run MadGraph
    if args.run_madgraph:
        assert args.mdir is not None and args.nevent is not None, 'Requires an exisiting MadGraph dir & number of events if running the MadGraph step'
        _logger.info(f'Runing MadGraph: mdir={args.mdir}, nevent={args.nevent}')
        with open(f'{args.mdir}/run_madgraph.dat', 'w') as fw:
            fw.write(f'launch {args.mdir} --name {args.output_tag}\n')
            fw.write('shower=OFF\n')
            fw.write('done\n')
            fw.write(f'set nevents {args.nevent}\n')
            fw.write('done\n')
        runcmd(
            cmd='{path}/bin/mg5_aMC {mdir}/run_madgraph.dat'.format(path=config['MG5_path'], mdir=args.mdir),
            logpath=os.path.join(args.mdir, f'madgraph_{args.output_tag}.log') if not args.testrun else None
        )
        _logger.info('MadGraph finished.')
        os.remove(f'{args.mdir}/run_madgraph.dat')
        assert os.path.exists(os.path.join(args.mdir, 'Events', args.output_tag, 'unweighted_events.lhe.gz')), 'MadGraph did not produce the LHE file..'

        runcmd(
            cmd=f'gzip -d {args.mdir}/Events/{args.output_tag}/unweighted_events.lhe.gz'.format(path=args.mdir, tag=args.output_tag),
        )
        # automatic input LHE file and Pythia card
        args.inputfile = f'{args.mdir}/Events/{args.output_tag}/unweighted_events.lhe'
        args.pcard = f'{args.mdir}/Events/pilotrun/PY8_parallelization/PY8Card.dat' ## obtain this card from a pilotrun MadGraph step!

    ## Split LHE events
    splitdir = 'mg5helper_split'
    if args.force and os.path.exists(os.path.join(os.path.dirname(args.inputfile), splitdir)):
        shutil.rmtree(os.path.join(os.path.dirname(args.inputfile), splitdir))
    lhe_split(path=args.inputfile, num_split=args.num_split, splitdir=splitdir)

    ## Run Pythia8 and Delphes
    pool = multiprocessing.Pool(processes=args.num_split)
    for i in range(args.num_split):
        pool.apply_async(
            run_simulation,
            args=(os.path.join(os.path.dirname(args.inputfile), splitdir, 'split_%d' % i), args, config)
        )
        if args.testrun:
            break
    _logger.info("Starting launch tasks...")
    pool.close()
    pool.join()
    _logger.info("All tasks completed...")

    if not args.testrun:
        ## Combine delphes file
        combine_root(os.path.dirname(args.inputfile), splitdir, args)
        ## Clean up
        shutil.rmtree(os.path.join(os.path.dirname(args.inputfile), splitdir))
