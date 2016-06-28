OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(_start)
SECTIONS
{
	. = 0x80000000;

	. = ALIGN(4);
	.text :	
	{
		*(.init)
		*(.text)
		*(.gnu.linkonce.t*)
		
		/* section information for finsh shell */
		. = ALIGN(4);
		__fsymtab_start = .;
		KEEP(*(FSymTab))
		__fsymtab_end = .;
		. = ALIGN(4);
		__vsymtab_start = .;
		KEEP(*(VSymTab))
		__vsymtab_end = .;
		. = ALIGN(4);	

		/* section information for modules */
		. = ALIGN(4);
		__rtmsymtab_start = .;
		KEEP(*(RTMSymTab))
		__rtmsymtab_end = .;

		. = ALIGN(4);
		__rt_init_start = .;
		KEEP(*(SORT(.rti_fn*)))
		__rt_init_end = .;
		. = ALIGN(4);
	}

	. = ALIGN(4);
	.rodata : { *(.rodata) *(.rodata.*) *(.gnu.linkonce.r*) *(.eh_frame) }

	. = ALIGN(4);
	.ctors :
	{
		PROVIDE(__ctors_start__ = .);
		/* old GCC version uses .ctors */
		KEEP(*(SORT(.ctors.*)))
		KEEP(*(.ctors))
		/* new GCC version uses .init_array */
		KEEP (*(SORT(.init_array.*)))
		KEEP (*(.init_array))
		PROVIDE(__ctors_end__ = .);
	}

	. = ALIGN(4);
	/* as a standalone section if you use ARM target.  */
	/* The .ARM.exidx section is used for C++ exception handling. */
	/* .ARM.exidx is sorted, so has to go in its own output section.  */
	ARM.exidx :
	{
		PROVIDE(__exidx_start = .);
		*(.ARM.exidx* .gnu.linkonce.armexidx.*)
		/* This is used by the startup in order to initialize the .data secion */
		_sidata = .;
		PROVIDE(__exidx_end = .);
	}

	. = ALIGN(4);
	.dtors :
	{
		PROVIDE(__dtors_start__ = .);
		KEEP(*(SORT(.dtors.*)))
		KEEP(*(.dtors))
		KEEP (*(SORT(.fini_array.*)))
		KEEP (*(.fini_array ))
		PROVIDE(__dtors_end__ = .);
	}

	. = ALIGN(4);
	.data :
	{
		*(.data)
		*(.data.*)
		*(.gnu.linkonce.d*)
	}

	. = ALIGN(4);
	.nobss : { *(.nobss) }


	. = ALIGN(4);
	__bss_start = .;
	.bss : { *(.bss) }
	__bss_end = .;

	/* stabs debugging sections. */
	.stab 0 : { *(.stab) }
	.stabstr 0 : { *(.stabstr) }
	.stab.excl 0 : { *(.stab.excl) }
	.stab.exclstr 0 : { *(.stab.exclstr) }
	.stab.index 0 : { *(.stab.index) }
	.stab.indexstr 0 : { *(.stab.indexstr) }
	.comment 0 : { *(.comment) }
	.debug_abbrev 0 : { *(.debug_abbrev) }
	.debug_info 0 : { *(.debug_info) }
	.debug_line 0 : { *(.debug_line) }
	.debug_pubnames 0 : { *(.debug_pubnames) }
	.debug_aranges 0 : { *(.debug_aranges) }

	_end = .;
}
