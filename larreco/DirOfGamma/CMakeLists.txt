
art_make( 
          
          LIB_LIBRARIES larcore_Geometry
                        larcore_Geometry_Geometry_service
                        larsim_Simulation
                        lardata_RawData
                        lardata_RecoBase
		        larreco_RecoAlg
			larreco_RecoAlg_PMAlg
                        lardata_Utilities
                        ${SIMULATIONBASE}
                        ${ART_FRAMEWORK_CORE}
                        ${ART_FRAMEWORK_PRINCIPAL}
                        ${ART_FRAMEWORK_SERVICES_REGISTRY}
                        ${ART_FRAMEWORK_SERVICES_BASIC}
                        ${ART_FRAMEWORK_SERVICES_OPTIONAL}
                        ${ART_PERSISTENCY_COMMON}
			${ART_PERSISTENCY_PROVENANCE}
                        ${ART_UTILITIES}
 			${MF_MESSAGELOGGER}
			${MF_UTILITIES}
                        ${FHICLCPP}
                        ${CETLIB}
                        ${ROOT_BASIC_LIB_LIST}
		        ${Boost_SYSTEM_LIBRARY}
	  MODULE_LIBRARIES larreco_RecoAlg
			   larreco_RecoAlg_PMAlg
			   larreco_DirOfGamma
			   larsim_MCCheater_BackTracker_service
      			   lardata_RecoBase
			   larsim_Simulation
			   larcore_Geometry
	    		   larcore_Geometry_Geometry_service
      			   lardata_Utilities
			   ${SIMULATIONBASE}
			   ${ART_FRAMEWORK_CORE}
			   ${ART_FRAMEWORK_PRINCIPAL}
			   ${ART_FRAMEWORK_SERVICES_REGISTRY}
			   ${ART_FRAMEWORK_SERVICES_BASIC}
			   ${ART_FRAMEWORK_SERVICES_OPTIONAL}
			   ${ART_FRAMEWORK_SERVICES_OPTIONAL_TFILESERVICE_SERVICE}
			   ${ART_PERSISTENCY_COMMON}
			   ${ART_PERSISTENCY_PROVENANCE}
			   ${ART_UTILITIES}
			   ${MF_MESSAGELOGGER}
			   ${MF_UTILITIES}
	      		   ${FHICLCPP}
			   ${CETLIB}
			   ${ROOT_BASIC_LIB_LIST}
)

install_headers()
install_fhicl()
install_source()
