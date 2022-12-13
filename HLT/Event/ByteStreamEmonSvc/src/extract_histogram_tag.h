/*
 Copyright (C) 2022 Komeilkma for the benefit of the ATLAS collaboration
*/

#include <string>
#include <utility>

namespace detail {

/**
 * Extract tag/LB number from per-LBN histogram name.
 *
 * This method supports naming conventions used in run2 and run3. It returns a
 * histogram hame without LB number in it and a tag (LBN) extracted from the
 * name. If histogram name does not have LBN in it then the name is returned
 * without any change, and tag is set to -1.
 */
std::pair<std::string, int> extract_histogram_tag(const std::string& histo_name);


} // namespace detail
