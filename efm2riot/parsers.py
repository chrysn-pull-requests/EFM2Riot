import glob
import os
import re


def parse_families(sdk_directory):
    """
    Index all available families from the SDK.
    """

    folders = glob.glob(os.path.join(sdk_directory, "Device/SiliconLabs/*"))
    families = []

    for folder in folders:
        if not os.path.isdir(folder):
            continue

        families.append({
            "family": os.path.basename(folder).lower(),
            "family_display_name": os.path.basename(folder).upper(),
        })

    return families


def parse_cpus(sdk_directory, family):
    """
    Index all available CPUs of a family. Parse the source files and for the
    information needed.
    """

    includes = glob.glob(os.path.join(
        sdk_directory,
        "Device/SiliconLabs/%(family)s/Include/*.h" % family)) + glob.glob(
            os.path.join(
                sdk_directory,
                "Device/SiliconLabs/%(family)s/Source/system_*.c" % family)
        )
    cpus = []

    hex_re = re.compile(r".*(0x\d+).*")
    irq_re = re.compile(r"\s*([a-zA-Z0-9_]+)\s* = (-?\d+).*")

    for include in includes:
        if not os.path.isfile(include):
            continue
        elif "_" in os.path.basename(include):
            continue
        else:
            cpu_platform = None
            flash_size = None
            ram_size = None
            architecture = None
            irqs = {}
            max_irq = None

            in_irq = False

            with open(include, "r") as fp:
                for line in fp:
                    if "#define" in line:
                        if "FLASH_SIZE" in line:
                            flash_size = int(hex_re.match(line).group(1), 16)
                        elif "SRAM_SIZE" in line:
                            ram_size = int(hex_re.match(line).group(1), 16)
                        elif "_SILICON_LABS_32B_PLATFORM_1" in line:
                            cpu_platform = 1
                        elif "_SILICON_LABS_32B_PLATFORM_2" in line:
                            cpu_platform = 2
                    elif "Cortex-M4" in line:
                        architecture = "m4"
                    elif "Cortex-M3" in line:
                        architecture = "m3"
                    elif "Cortex-M0+" in line:
                        architecture = "m0plus"
                    elif "Cortex-M0" in line:
                        architecture = "m0"
                    elif "typedef enum IRQn" in line:
                        in_irq = True

                    if in_irq:
                        if "} IRQn_Type;" in line:
                            in_irq = False

                        irq = irq_re.match(line)

                        if irq:
                            irqs[int(irq.group(2))] = irq.group(1)
                            max_irq = int(irq.group(2))

        if not flash_size or not ram_size:
            raise Exception("Missing flash/ram size in include %s" % include)

        # Fix the IRQ to be a full list
        irq_table = []

        for number in range(max_irq):
            if number in irqs:
                name = irqs[number]

                irq_name = name.replace("_IRQn", "")
                method_name = "isr_" + irq_name.lower()

                irq_table.append({
                    "reserved": False,
                    "method_name": method_name,
                    "name": irq_name,
                    "number": number
                })
            else:
                irq_table.append({
                    "reserved": True
                })

        cpu = {
            "cpu": os.path.basename(include).split(".")[0],
            "cpu_platform": cpu_platform,
            "flash_size": flash_size,
            "ram_size": ram_size
        }

        family.update({
            "architecture": architecture,
            "irqs": irq_table,
            "max_irq": max_irq,
            "max_irq_name": irqs[max_irq]
        })
        cpu.update(family)
        cpus.append(cpu)

    return cpus
