



prg.update: function() {
				if ( !programEdited ){
					program_status = [];
					for ( var d = 0; d < sts.status.program.length; d++ ){
						program_status[d] = [];
						for ( var h = 0; h < sts.status.program[d].length; h++ )                                
							program_status[d][h] = sts.status.program[d][h];
					}
					programRefresh();
				}
}