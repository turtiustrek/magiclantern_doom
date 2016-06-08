# Parse mpu_send/mpu_recv logs (from dm-spy-experiments branch)
# and generate MPU init spells code for QEMU.
#
# Very rough proof of concept, far from complete. Tested on 60D.

import os, sys, re

comment = True
first_mpu_send_only = False

def format_spell(spell):
    bytes = spell.split(" ")
    bytes = ["0x" + b for b in bytes]
    return "{ " + ", ".join(bytes) + " },"

f = open(sys.argv[1], "r")

print "struct mpu_init_spell mpu_init_spells[] = { {"
first_block = True
num = 1
num2 = 1

first_send = True
waitid_prop = None

q_output = ""

for l in f.readlines():
    m = re.match(".* mpu_send\(([^()]*)\)", l)
    if m:
        spell = m.groups()[0]

        if first_send or not first_mpu_send_only:
            first_send = False
            
            # do not output empty mpu_send blocks right away
            # rather, queue their output, and write it only if a corresponding mpu_recv is found
            q_output = ""
            
            if first_block: first_block = False
            else: q_output += "        { 0 } } }, {" + "\n"
            
            if waitid_prop:
                assert spell.strip().startswith("08 06 00 00 ")
                q_output += "    %-60s/* spell #%d, Complete WaitID = %s */" % (format_spell(spell) + " {", num, waitid_prop) + "\n"
                waitid_prop = None
                continue
            
            if comment: q_output += "    %-60s/* spell #%d */" % (format_spell(spell) + " {", num) + "\n"
            else:       q_output += "    " + format_spell(spell) + " {" + "\n"
            continue

    m = re.match(".* mpu_recv\(([^()]*)\)", l)
    if m:
        spell = m.groups()[0]
        
        if q_output:
            print q_output,
            q_output = ""
            num += 1
            num2 = 1
        
        if comment: print "        %-56s/* reply #%d.%d */" % (format_spell(spell), num-1, num2)
        else: print "        " + format_spell(spell)
        num2 += 1
        continue
    
    # after a Complete WaitID line, the ICU sends to the MPU a message saying it's ready
    # so the MPU can then send data for the next property that requires a "Complete WaitID"
    # (if those are not synced, you will get ERROR TWICE ACK REQUEST)
    # example:
    #    PropMgr:ff31ec3c:01:03: Complete WaitID = 0x80020000, 0xFF178514(0)
    #    PropMgr:00c5c318:00:00: *** mpu_send(08 06 00 00 04 00 00), from 616c
    # the countdown at the end of the line must be 0

    m = re.match(".*Complete WaitID = ([0-9A-Fx]+), ([0-9A-Fx]+)\(0\)", l)
    if m:
        waitid_prop = m.groups()[0]

print "    { 0 } } }"
print "};"
