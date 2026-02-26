#include <vector>
#include <fstream>
#include <algorithm>
#include <cmath>
#include <cstring>

// Compute a "nice" rounded maximum for the y-axis
static double nice_max(double v) {
    if (v <= 0.0) return 1.0;

    double exp = std::floor(std::log10(v));
    double f   = v / std::pow(10.0, exp);

    double nf;
    if (f <= 1.0)      nf = 1.0;
    else if (f <= 2.0) nf = 2.0;
    else if (f <= 5.0) nf = 5.0;
    else               nf = 10.0;

    return nf * std::pow(10.0, exp);
}

void write_svg_line_chart(
    const char* filename,
    const char* title,
    const char* x_axis_label,
    const char* y_axis_label,
    const std::vector<std::vector<double>>& data, // data[series][group]
    const char* const* series_labels,
    const char* const* x_tick_labels
) {
    const size_t series_count = data.size();
    if (series_count == 0) return;

    const size_t group_count = data[0].size();
    if (group_count < 2) return;

    for (const auto& s : data)
        if (s.size() != group_count)
            return;

    // Determine Y scale
    double max_value = 0.0;
    for (const auto& s : data)
        for (double v : s)
            max_value = std::max(max_value, v);

    const double y_max  = nice_max(max_value);
    const int y_ticks   = 5;

    // Layout
    const int width  = 900;
    const int height = 600;

    const int margin_left   = 90;
    const int margin_right  = 30;
    const int margin_top    = 60;
    const int margin_bottom = 120;

    const int plot_width  = width  - margin_left - margin_right;
    const int plot_height = height - margin_top  - margin_bottom;

    const int x_axis_y = margin_top + plot_height;

    std::ofstream out(filename);
    if (!out) return;

    out << "<svg xmlns=\"http://www.w3.org/2000/svg\" "
        << "width=\"" << width << "\" height=\"" << height << "\">\n";

    out << "<style>"
        << "text { font-family: sans-serif; fill: #000; }"
        << "</style>\n";

    // Title
    if (title && *title) {
        out << "<text x=\"" << width / 2
            << "\" y=\"30\" text-anchor=\"middle\" "
            << "font-size=\"18\" font-weight=\"bold\">"
            << title << "</text>\n";
    }

    // Axes
    out << "<line x1=\"" << margin_left
        << "\" y1=\"" << x_axis_y
        << "\" x2=\"" << margin_left + plot_width
        << "\" y2=\"" << x_axis_y
        << "\" stroke=\"black\" />\n";

    out << "<line x1=\"" << margin_left
        << "\" y1=\"" << margin_top
        << "\" x2=\"" << margin_left
        << "\" y2=\"" << x_axis_y
        << "\" stroke=\"black\" />\n";

    // Y ticks and labels
    for (int i = 0; i <= y_ticks; ++i) {
        double value = y_max * i / y_ticks;
        int y = margin_top + plot_height
              - static_cast<int>(plot_height * i / y_ticks);

        out << "<line x1=\"" << margin_left - 5
            << "\" y1=\"" << y
            << "\" x2=\"" << margin_left
            << "\" y2=\"" << y
            << "\" stroke=\"black\" />\n";

        out << "<text x=\"" << margin_left - 10
            << "\" y=\"" << y + 4
            << "\" text-anchor=\"end\" font-size=\"12\">"
            << static_cast<long long>(value)
            << "</text>\n";
    }

    // Y-axis label (vertical)
    if (y_axis_label && *y_axis_label) {
        const int y_label_x = margin_left - 60;
        const int y_label_y = margin_top + plot_height / 2;

        out << "<text x=\"" << y_label_x
            << "\" y=\"" << y_label_y
            << "\" text-anchor=\"middle\" "
            << "transform=\"rotate(-90 "
            << y_label_x << "," << y_label_y << ")\">"
            << y_axis_label << "</text>\n";
    }

    const char* colors[] = {
        "#4e79a7", "#f28e2b", "#e15759",
        "#76b7b2", "#59a14f", "#edc949"
    };

    // Lines and points
    for (size_t s = 0; s < series_count; ++s) {
        out << "<polyline fill=\"none\" stroke=\""
            << colors[s % 6]
            << "\" stroke-width=\"2\" points=\"";

        for (size_t g = 0; g < group_count; ++g) {
            double x_frac = double(g) / double(group_count - 1);
            double x = margin_left + x_frac * plot_width;

            double y = margin_top + plot_height
                     - (data[s][g] / y_max) * plot_height;

            out << x << "," << y << " ";
        }

        out << "\" />\n";

        // Point markers
        for (size_t g = 0; g < group_count; ++g) {
            double x_frac = double(g) / double(group_count - 1);
            double x = margin_left + x_frac * plot_width;

            double y = margin_top + plot_height
                     - (data[s][g] / y_max) * plot_height;

            out << "<circle cx=\"" << x
                << "\" cy=\"" << y
                << "\" r=\"3\" fill=\""
                << colors[s % 6] << "\" />\n";
        }
    }

    // X tick labels
    for (size_t g = 0; g < group_count; ++g) {
        double x_frac = double(g) / double(group_count - 1);
        double x = margin_left + x_frac * plot_width;

        out << "<text x=\"" << x
            << "\" y=\"" << x_axis_y + 20
            << "\" text-anchor=\"middle\" font-size=\"12\">"
            << x_tick_labels[g]
            << "</text>\n";
    }

    // X-axis label (immediately under ticks)
    if (x_axis_label && *x_axis_label) {
        out << "<text x=\"" << margin_left + plot_width / 2
            << "\" y=\"" << x_axis_y + 45
            << "\" text-anchor=\"middle\">"
            << x_axis_label << "</text>\n";
    }

    // Legend (below x-axis label)
    const int legend_y = height - 30;
    int legend_x = width / 2 - static_cast<int>(series_count) * 120 / 2;

    for (size_t s = 0; s < series_count; ++s) {
        out << "<line x1=\"" << legend_x
            << "\" y1=\"" << legend_y
            << "\" x2=\"" << legend_x + 20
            << "\" y2=\"" << legend_y
            << "\" stroke=\"" << colors[s % 6]
            << "\" stroke-width=\"3\" />\n";

        out << "<text x=\"" << legend_x + 26
            << "\" y=\"" << legend_y + 4
            << "\" font-size=\"12\">"
            << series_labels[s]
            << "</text>\n";

        legend_x += 120;
    }

    out << "</svg>\n";
}
