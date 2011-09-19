# -*- mode: python; indent-tabs-mode: nil; tab-width: 3 -*-
# vim: set tabstop=3 shiftwidth=3 expandtab:
#
# Copyright (C) 2003 Karl MacMillan
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#

from gamera.core import *
from gamera import knn, gamera_xml, cluster, stats
import math
init_gamera()

def max_distance(glyphs, k):
   ud = k.unique_distances(glyphs, 0)
   ud.sort()
   return ud[-1]

def local_max_distance(glyphs, x, k):
   d = k.distance_from_images(glyphs, x)
   d.sort()
   return d[-1][0]

def get_graph_stats(glyphs, k):
   s = { }
   gc = knn.glyphs_by_category(glyphs)
   for x in gc.itervalues():
      if len(x) == 1:
         s[x[0].get_main_id()] = 1.0
         continue
      graph = cluster.make_spanning_tree(x, k)
      edges = []
      for edge in graph.get_edges():
         edges.append(edge.cost)
      id = x[0].get_main_id()
      s[id] = stats.mean(edges)
      #cluster.make_subtrees_stddev(graph, 1, 2, 0)
      #print graph.nedges, len(x)
   return s

def strip_small_categories(glyphs):
   gc = knn.glyphs_by_category(glyphs)
   new_glyphs = []
   for v in gc.itervalues():
      if len(v) > 3:
         new_glyphs.extend(v)
   return new_glyphs
         

def test():
   glyphs = gamera_xml.glyphs_from_xml(r"C:\Documents and Settings\Karl MacMillan\Desktop\test\prod.xml")

   glyphs = strip_small_categories(glyphs)
   from gamera.plugins import features
   k = knn.kNN()
   print k.features
   features.generate_features_list(glyphs, k.feature_functions)
   print "Getting gstats"

   graph_stats = get_graph_stats(glyphs, k)
   gstats = knn.get_glyphs_stats(glyphs)

   max_dist = max_distance(glyphs, k)
   print max_dist
   file = open("results.txt", "w")
   global_max = [[],[]]
   local_max = [[],[]]
   all = [[],[]]
   graph = [[],[]]
   gr_ccorrect = 0
   gr_icorrect = 0
   for x in glyphs:
      local_max_dist = local_max_distance(glyphs, x, k)
      ans = k.classify_with_images(glyphs, x, 1)
      file.write(ans[0][1] + ",")# + str(ans[0][0]) + ",")
      correct = 0
      if x.get_main_id() == ans[0][1]:
         file.write("1,")
         correct = 1
      else:
         file.write("0,")
      g = 1.0 - (ans[0][0] / max_dist)
      global_max[correct].append(g)
      file.write(str(g) + ",")

      l = 1.0 - (ans[0][0] / local_max_dist)
      local_max[correct].append(l)
      file.write(str(l) + ",")

      a = stats.samplestdev([ans[0][0],gstats[ans[0][1]][1]])
      all[correct].append(a)
      file.write(str(a) + ",")

      gr = stats.samplestdev([ans[0][0],graph_stats[ans[0][1]]])
      if (gr <= 1 and correct):
         gr_ccorrect += 1
      if (gr > 1 and not correct):
         gr_icorrect += 1
      graph[correct].append(gr)
      file.write(str(gr))

      file.write("\n")

   print "num correct: %d num incorrect: %d" % (len(global_max[1]), len(global_max[0]))
   print "confidence %f %f %f" % (((gr_ccorrect + gr_icorrect) / float(len(glyphs))),
                                  gr_ccorrect / float(len(glyphs) - len(global_max[0])),
                                  gr_icorrect / float(len(glyphs) - len(global_max[1])))

   cgm = -1
   igm = -1
   cgs = -1
   igs = -1
   if (len(global_max[0])):
      igm = stats.mean(global_max[0])
      igs = stats.samplestdev(global_max[0])
   if (len(global_max[1])):
      cgm = stats.mean(global_max[1])
      cgs = stats.samplestdev(global_max[1])

   clm = -1
   ilm = -1
   cls = -1
   ils = -1
   if (len(local_max[0])):
      ilm = stats.mean(local_max[0])
      ils = stats.samplestdev(local_max[0])
   if (len(local_max[1])):
      clm = stats.mean(local_max[1])
      cls = stats.samplestdev(local_max[1])

   cam = -1
   iam = -1
   cas = -1
   ias = -1
   if (len(all[0])):
      iam = stats.mean(all[0])
      ias = stats.samplestdev(all[0])
   if (len(all[1])):
      cam = stats.mean(all[1])
      cas = stats.samplestdev(all[1])

   cgraphm = -1
   igraphm = -1
   cgraphs = -1
   igraphs = -1
   if (len(graph[0])):
      igraphm = stats.mean(graph[0])
      igraphs = stats.samplestdev(graph[0])
   if (len(graph[1])):
      cgraphm = stats.mean(graph[1])
      cgraphs = stats.samplestdev(graph[1])

   print "global correct avg: %f stdev: %f incorrect avg: %f stddev: %f" % (cgm, cgs, igm, igs)
   print "local correct avg: %f stdev: %f incorrect avg: %f stddev: %f" % (clm, cls, ilm, ils)
   print "all correct avg: %f stdev: %f incorrect avg: %f stddev: %f" % (cam, cas, iam, ias)
   print "graph correct avg: %f stdev: %f incorrect avg: %f stddev: %f" % (cgraphm, cgraphs, igraphm, igraphs)

   def otsu_threshold(p):
      l = len(p)
      mu_T = 0.0
      for i in range(l):
         mu_T += i * p[i]

      sigma_T = 0.0
      for i in range(l):
         sigma_T += (i-mu_T)*(i-mu_T)*p[i]

      k_low = 0
      while (p[k_low] == 0) and (k_low < (l - 1)):
         k_low += 1
      k_high = l - 1
      while (p[k_high] == 0) and (k_high > 0):
         k_low += 1
         k_high -= 1

      criterion = 0.0
      thresh = 127

      omega_k = 0.0
      mu_k = 0.0
      k = k_low
      while k <= k_high:
         omega_k += p[k]
         mu_k += k*p[k]

         expr_1 = (mu_T*omega_k - mu_k)
         sigma_b_k = expr_1 * expr_1 / (omega_k*(1-omega_k))
         if (criterion < sigma_b_k/sigma_T):
            criterion = sigma_b_k/sigma_T
            thresh = k;
         k += 1
      return thresh

   graph_l = graph[0][:]
   graph_l.extend(graph[1])
   graph_l.sort()
   threshold = stats.mean(graph_l)
   print "threshold: " + str(threshold)
   num_wrong = 0
   for x in graph[0]:
      if x < threshold:
         num_wrong += 1
   print num_wrong, num_wrong / float(len(graph[0])) * 100

   num_wrong = 0
   for x in graph[1]:
      if x >= threshold:
         num_wrong += 1
   print num_wrong, num_wrong / float(len(graph[1])) * 100

   graph_l = all[0][:]
   graph_l.extend(all[1])
   graph_l.sort()
   threshold = stats.mean(graph_l)
   print "threshold: " + str(threshold)
   num_wrong = 0
   for x in graph[0]:
      if x < threshold:
         num_wrong += 1
   print num_wrong, num_wrong / float(len(graph[0])) * 100

   num_wrong = 0
   for x in graph[1]:
      if x >= threshold:
         num_wrong += 1
   print num_wrong, num_wrong / float(len(graph[1])) * 100

   graph_l = local_max[0][:]
   graph_l.extend(local_max[1])
   graph_l.sort()
   threshold = stats.mean(graph_l)
   print "threshold: " + str(threshold)
   num_wrong = 0
   for x in graph[0]:
      if x < threshold:
         num_wrong += 1
   print num_wrong, num_wrong / float(len(graph[0])) * 100

   num_wrong = 0
   for x in graph[1]:
      if x >= threshold:
         num_wrong += 1
   print num_wrong, num_wrong / float(len(graph[1])) * 100


test()

   

