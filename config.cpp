#include "config.hpp"

Json::Value defaultConfig(){
	Json::Value v;
	
	v["filepath_completion_use_working_dir"] = false;
	v["auto_trigger"] = true;
	v["min_num_of_chars_for_completion"] = 2;
	v["min_num_identifier_candidate_chars"] = 0;
	v["semantic_triggers"] = Json::Value(Json::objectValue);

	Json::Value ft_dis(Json::objectValue);
	ft_dis["gitcommit"] = true;

	v["filetype_specific_completion_to_disable"] = ft_dis;

	v["seed_identifiers_with_syntax"] = false;
	v["collect_identifiers_from_comments_and_strings"] = false;
	v["collect_identifiers_from_tags_files"] = false;
	v["extra_conf_globlist"] = Json::Value(Json::arrayValue);
	v["global_ycm_extra_conf"] = "";
	v["confirm_extra_conf"] = true;
	v["complete_in_comments"] = false;
	v["complete_in_strings"] = true;
	v["max_diagnostics_to_display"] = 30;

	Json::Value ft_w(Json::objectValue);
	ft_w["*"] = true;

	v["filetype_whitelist"] = ft_w;

	Json::Value ft_b(Json::objectValue);
	ft_b["tagbar"] = true;
	ft_b["qf"] = true;
	ft_b["notes"] = true;
	ft_b["markdown"] = true;
	ft_b["unite"] = true;
	ft_b["text"] = true;
	ft_b["vimwiki"] = true;
	ft_b["pandoc"] = true;
	ft_b["infolog"] = true;
	ft_b["mail"] = true;

	v["filetype_blacklist"] = ft_b;

	v["auto_start_csharp_server"] = true;
	v["auto_stop_csharp_server"] = true;
	v["use_ultisnips_completer"] = true;
	v["csharp_server_port"] = 2000;
	v["server_keep_logfiles"] = false;
	v["ycmd_path"] = "";

	return v;
}